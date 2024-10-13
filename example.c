#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"

int main() {
    // Create a new hashtable
    Hashtable *ht = db_open(INITIAL_TABLE_SIZE);

    // Insert some key-value pairs
    const char *key1 = "key1";
    const char *value1 = "value1";
    db_insert(ht, key1, (void *)value1, strlen(value1) + 1); // +1 for null terminator

    const char *key2 = "key2";
    int value2 = 42;
    db_insert(ht, key2, &value2, sizeof(value2));

    // Lookup values
    size_t size;
    char *str_value = (char *)db_lookup(ht, key1, &size);
    if (str_value) {
        printf("Value for %s: %s\n", key1, str_value);
        free(str_value);
    }

    int *int_value = (int *)db_lookup(ht, key2, &size);
    if (int_value) {
        printf("Value for %s: %d\n", key2, *int_value);
        free(int_value);
    }

    // Serialize the hashtable to a file
    db_serialize(ht, "hashtable.bin");

    // Close the current hashtable
    db_close(ht);

    // Create a new hashtable and deserialize from file
    Hashtable *new_ht = db_open(INITIAL_TABLE_SIZE);
    db_deserialize(new_ht, "hashtable.bin");

    // Check deserialized values
    str_value = (char *)db_lookup(new_ht, key1, &size);
    if (str_value) {
        printf("Deserialized value for %s: %s\n", key1, str_value);
        free(str_value);
    }

    int_value = (int *)db_lookup(new_ht, key2, &size);
    if (int_value) {
        printf("Deserialized value for %s: %d\n", key2, *int_value);
        free(int_value);
    }

    // Delete an entry
    db_delete(new_ht, key1);

    // Try to lookup the deleted entry
    str_value = (char *)db_lookup(new_ht, key1, &size);
    if (!str_value) {
        printf("%s was successfully deleted.\n", key1);
    }

    // Close the new hashtable
    db_close(new_ht);

    return 0;
}