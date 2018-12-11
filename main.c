#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "lru.h"

#define XXH_INLINE_ALL
#include "xxHash/xxhash.h"

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

struct demo_obj {
	struct lru_node node;
	char name[32];
};

struct lru_node *demo_obj_init(struct lru_node *obj, void *key)
{
	struct demo_obj *a = container_of(obj, struct demo_obj, node);
	struct demo_obj *b = (struct demo_obj *)key;
	printf("Initializing object '%s' (%016lx)\n", b->name, b->node.hash);
	memcpy(a, b, sizeof(struct demo_obj));
	return obj;
}

struct lru_node *demo_obj_deinit(struct lru_node *obj)
{
	struct demo_obj *a = container_of(obj, struct demo_obj, node);
	printf("Deinitializing object '%s'\n", a->name);
	return obj;
}

int demo_obj_key_compare(struct lru_node *obj, void *key)
{
	struct demo_obj *a = container_of(obj, struct demo_obj, node);
	struct demo_obj *b = (struct demo_obj *)key;

	printf("Comparing %s == %s?\n", a->name, b->name);
	return memcmp(a->name, b->name, sizeof(a->name));
}

const int num_test_strings = 10;
char *test_strings[10] = {
	"Apples",
	"Bananas",
	"Pears",
	"Cucumber",
	"Pickle",
	"Pineapple",
	"Pizza",
	"Soda",
	"Cereal",
	"Coffee",
};

int main(int argc, char *argv[])
{
	struct lru lru;

	lru_init(&lru, &demo_obj_init, &demo_obj_deinit, &demo_obj_key_compare);

	/* Allocate objects for cache entries */
	const size_t num_cache_entries = 8;
	printf("# Cache Entries: %zd\n", num_cache_entries);
	
	/* Add objects to the cache */
	struct demo_obj *cache_objects = malloc(num_cache_entries * sizeof(struct demo_obj));
	assert(cache_objects != NULL);
	for (size_t i = 0; i < num_cache_entries; i++) {
		printf("Adding entry %zd\n", i);
		lru_add_free(&lru, &cache_objects[i].node);
	}

	struct demo_obj key;
	struct lru_node *node;

	for (size_t i = 0; i < num_test_strings; i++) {
		memset(&key, 0, sizeof(struct demo_obj));

		strncpy(key.name, test_strings[i], sizeof(key.name));
		key.node.hash = XXH64(key.name, strlen(key.name), 0);

		printf("Looking up...\n");
		node = lru_lookup(&lru, key.node.hash, &key);
		printf("node = %p\n", node);
	}

#if 1
	printf("Trying in reverse...\n");
	for (int i = num_test_strings-1; i >= 0; i--)
#else
	printf("Now starting over...\n");
	for (size_t i = 0; i < num_test_strings; i++)
#endif
	{
		memset(&key, 0, sizeof(struct demo_obj));
		strncpy(key.name, test_strings[i], sizeof(key.name));
		key.node.hash = XXH64(key.name, strlen(key.name), 0);
		printf("Looking up %s...\n", key.name);
		node = lru_lookup(&lru, key.node.hash, &key);
		printf("node = %p\n", node);
	}

	lru.num_miss -= num_test_strings;

	float hit_percent = 100.0f * (float)lru.num_hit / (float)(lru.num_hit + lru.num_miss);
	printf("Hit:Miss = %zd:%zd (%.2f%%)\n", lru.num_hit, lru.num_miss, hit_percent);

	/* Flush the cache on free the cache entry objects */
	lru_flush(&lru);
	free(cache_objects);

	return 0;
}
