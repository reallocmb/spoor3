#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>

typedef struct {
    FILE *f;
    uint32_t size;
    uint32_t items;
} RedbasDB;

RedbasDB *redbas_db_open(char *path, uint32_t size)
{
    RedbasDB *redbas_db = malloc(sizeof(*redbas_db));
    redbas_db->f = fopen(path, "rb");
    if (redbas_db->f == NULL)
    {
        redbas_db->f = fopen(path, "w+b");
        if (redbas_db->f == NULL)
            return NULL;
        redbas_db->size = size;
        fwrite(&size, sizeof(size), 1, redbas_db->f);
        redbas_db->items = 0;
        fwrite(&redbas_db->items, sizeof(redbas_db->items), 1, redbas_db->f);
        fclose(redbas_db->f);
        redbas_db->f = fopen(path, "r+b");
    }
    else
    {
        fclose(redbas_db->f);
        redbas_db->f = fopen(path, "r+b");
        fread(&redbas_db->size, sizeof(redbas_db->size), 1, redbas_db->f);
        fread(&redbas_db->items, sizeof(redbas_db->items), 1, redbas_db->f);
    }

    return redbas_db;
}

uint32_t redbas_db_items(RedbasDB *db)
{
    return db->items;
}

void redbas_db_store(RedbasDB *db, void *data, uint32_t size)
{
    if (db->size == size)
    {
        fseek(db->f, 0, SEEK_END);
        fwrite(data, size, 1, db->f);
        db->items++;
    }
}

void redbas_db_change(RedbasDB *db, void *data, uint32_t size, uint32_t index)
{
    if (db->size == size)
    {
        fseek(db->f, index * db->size + sizeof(db->size) + sizeof(db->items), SEEK_SET);
        fwrite(data, size, 1, db->f);
    }
}

void redbas_db_restore_cursor_set(RedbasDB *db, uint32_t index)
{
    fseek(db->f, index * db->size + sizeof(db->items) + sizeof(db->size), SEEK_SET);
}

void redbas_db_restore(RedbasDB *db, void *data, uint32_t size)
{
    if (db->size == size)
    {
        fread(data, size, 1, db->f);
    }
}

void redbas_db_close(RedbasDB *db)
{
    fseek(db->f, sizeof(db->size), SEEK_SET);
    fwrite(&db->items, sizeof(db->items), 1, db->f);
    fclose(db->f);
    free(db);
}
