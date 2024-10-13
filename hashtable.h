// Concurrent safe, in-memory persistent hashtable
// BSD 3-Clause License
// Copyright (c) Alex Gaetano Padula

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.

// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define INITIAL_TABLE_SIZE 128
#define LOAD_FACTOR_THRESHOLD 0.75

typedef struct Entry {
    char *key;           
    void *value;        
    size_t value_size;  
    struct Entry *next;  
} Entry;

typedef struct Hashtable {
    Entry **table;         
    pthread_mutex_t *locks; 
    size_t size;          
    size_t count;         
} Hashtable;

// Hash function
unsigned int hash(const char *key, size_t table_size) {
    unsigned int hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % table_size;
}

// Create a hashtable
Hashtable *create_hashtable(size_t initial_size) {
    Hashtable *ht = malloc(sizeof(Hashtable));
    ht->table = calloc(initial_size, sizeof(Entry *));
    ht->locks = malloc(sizeof(pthread_mutex_t) * initial_size);
    ht->size = initial_size;
    ht->count = 0;

    for (size_t i = 0; i < initial_size; i++) {
        pthread_mutex_init(&ht->locks[i], NULL);
    }

    return ht;
}

// Free hashtable
void free_hashtable(Hashtable *ht) {
    for (size_t i = 0; i < ht->size; i++) {
        pthread_mutex_lock(&ht->locks[i]);
        Entry *entry = ht->table[i];
        while (entry) {
            Entry *temp = entry;
            entry = entry->next;
            free(temp->key);
            free(temp->value);
            free(temp);
        }
        pthread_mutex_unlock(&ht->locks[i]);
        pthread_mutex_destroy(&ht->locks[i]);
    }
    free(ht->locks);
    free(ht->table);
    free(ht);
}

// Resize the hashtable
void resize(Hashtable *ht) {
    size_t new_size = ht->size * 2;
    Entry **new_table = calloc(new_size, sizeof(Entry *));
    pthread_mutex_t *new_locks = malloc(sizeof(pthread_mutex_t) * new_size);

    for (size_t i = 0; i < new_size; i++) {
        pthread_mutex_init(&new_locks[i], NULL);
    }

    for (size_t i = 0; i < ht->size; i++) {
        pthread_mutex_lock(&ht->locks[i]);
        Entry *entry = ht->table[i];
        while (entry) {
            unsigned int new_index = hash(entry->key, new_size);
            Entry *next_entry = entry->next;

            entry->next = new_table[new_index];
            new_table[new_index] = entry;

            entry = next_entry;
        }
        pthread_mutex_unlock(&ht->locks[i]);
    }

    free(ht->table);
    free(ht->locks);

    ht->table = new_table;
    ht->locks = new_locks;
    ht->size = new_size;
}

// Insert or update a key-value pair
int db_insert(Hashtable *ht, const char *key, void *value, size_t value_size) {
    if ((double)ht->count / ht->size > LOAD_FACTOR_THRESHOLD) {
        resize(ht);
    }

    unsigned int index = hash(key, ht->size);
    pthread_mutex_lock(&ht->locks[index]);

    Entry *entry = ht->table[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            free(entry->value);
            entry->value = malloc(value_size);
            memcpy(entry->value, value, value_size);
            entry->value_size = value_size;
            pthread_mutex_unlock(&ht->locks[index]);
            return 0; // Success
        }
        entry = entry->next;
    }

    Entry *new_entry = malloc(sizeof(Entry));
    new_entry->key = strdup(key);
    new_entry->value = malloc(value_size);
    memcpy(new_entry->value, value, value_size);
    new_entry->value_size = value_size;
    new_entry->next = ht->table[index];
    ht->table[index] = new_entry;
    ht->count++;

    pthread_mutex_unlock(&ht->locks[index]);
    return 0; // Success
}

// Lookup a key
void *db_lookup(Hashtable *ht, const char *key, size_t *value_size) {
    unsigned int index = hash(key, ht->size);
    pthread_mutex_lock(&ht->locks[index]);

    Entry *entry = ht->table[index];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            void *value = malloc(entry->value_size);
            memcpy(value, entry->value, entry->value_size);
            *value_size = entry->value_size; 
            pthread_mutex_unlock(&ht->locks[index]);
            return value; 
        }
        entry = entry->next;
    }

    pthread_mutex_unlock(&ht->locks[index]);
    return NULL; 
}

// Delete a key-value pair
int db_delete(Hashtable *ht, const char *key) {
    unsigned int index = hash(key, ht->size);
    pthread_mutex_lock(&ht->locks[index]);

    Entry *entry = ht->table[index];
    Entry *prev = NULL;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                ht->table[index] = entry->next;
            }
            free(entry->key);
            free(entry->value);
            free(entry);
            ht->count--;
            pthread_mutex_unlock(&ht->locks[index]);
            return 0; // Success
        }
        prev = entry;
        entry = entry->next;
    }

    pthread_mutex_unlock(&ht->locks[index]);
    return -1; // Key not found
}

// Serialize hashtable to a file
int db_serialize(Hashtable *ht, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        return -1; 
    }

    for (size_t i = 0; i < ht->size; i++) {
        pthread_mutex_lock(&ht->locks[i]);
        Entry *entry = ht->table[i];
        while (entry) {
            size_t key_length = strlen(entry->key) + 1; 
            fwrite(&key_length, sizeof(size_t), 1, file);
            fwrite(entry->key, sizeof(char), key_length, file);
            fwrite(&entry->value_size, sizeof(size_t), 1, file);
            fwrite(entry->value, 1, entry->value_size, file);
            entry = entry->next;
        }
        pthread_mutex_unlock(&ht->locks[i]);
    }

    fclose(file);
    return 0; // Success
}

// Deserialize hashtable from a file
int db_deserialize(Hashtable *ht, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file for reading");
        return -1; 
    }

    while (!feof(file)) {
        size_t key_length;
        if (fread(&key_length, sizeof(size_t), 1, file) != 1) break;

        char *key = malloc(key_length);
        fread(key, sizeof(char), key_length, file);

        size_t value_size;
        fread(&value_size, sizeof(size_t), 1, file);
        void *value = malloc(value_size);
        fread(value, 1, value_size, file);

        db_insert(ht, key, value, value_size);
        free(key);
        free(value);
    }

    fclose(file);
    return 0; // Success
}

// Open a new hashtable
Hashtable *db_open(size_t initial_size) {
    return create_hashtable(initial_size);
}

// Close the hashtable
void db_close(Hashtable *ht) {
    free_hashtable(ht);
}

#endif // HASHTABLE_H
