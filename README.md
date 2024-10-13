# hashtable
Concurrent safe, in-memory hash-table with persistence and simple API.


### Create a Hashtable
```
Hashtable *db_open(size_t initial_size);
```

#### Params
`initial_size` The initial size of the hashtable.

### Free a Hashtable
```
void db_close(Hashtable *ht);
```

#### Params
`ht` Pointer to the hashtable to be freed.


### Insertion
```
int db_insert(Hashtable *ht, const char *key, void *value, size_t value_size);
```

#### Params
`ht` Pointer to the hashtable.

`key` The key to insert or update.

`value` Pointer to the value to associate with the key.

`value_size` Size of the value.

### Lookup
```
void *db_lookup(Hashtable *ht, const char *key, size_t *value_size);
```

#### Params
`ht` Pointer to the hashtable.

`key` The key to look up.

`value_size` Pointer to store the size of the retrieved value.

### Delete
```
int db_delete(Hashtable *ht, const char *key);
```

#### Params
`ht` Pointer to the hashtable.

`key` The key to delete.

### Serialization and Deserialization
```
int db_serialize(Hashtable *ht, const char *filename);
```

```
int db_deserialize(Hashtable *ht, const char *filename);
```

#### Params
`ht` Pointer to the hashtable.

`filename` The name of the file to read from.


### Example 
```
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"

int main() {
    Hashtable *ht = db_open(INITIAL_TABLE_SIZE);
    const char *key1 = "key1";
    const char *value1 = "value1";
    db_insert(ht, key1, (void *)value1, strlen(value1) + 1);

    size_t size;
    char *retrieved_value = (char *)db_lookup(ht, key1, &size);
    if (retrieved_value) {
        printf("Retrieved value: %s\n", retrieved_value);
        free(retrieved_value);
    }

    db_serialize(ht, "hashtable.bin");
    db_close(ht);

    Hashtable *new_ht = db_open(INITIAL_TABLE_SIZE);
    db_deserialize(new_ht, "hashtable.bin");

    // Clean up
    db_close(new_ht);
    return 0;
}
```

#### Compilation
```
gcc -o hashtable_example main.c -lpthread
```