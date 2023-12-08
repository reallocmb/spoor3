#ifndef REDBAS_H
#define REDBAS_H

#define REDBAS_VERSION_MAJOR 0
#define REDBAS_VERSION_MINOR 0
#define REDBAS_VERSION_PATCH 0

#include<stdint.h>

typedef void RedbasDB;

RedbasDB *redbas_db_open(char *path, uint32_t size);
uint32_t redbas_db_items(RedbasDB *db);
void redbas_db_store(RedbasDB *db, void *data, uint32_t size);
void redbas_db_change(RedbasDB *db, void *data, uint32_t size, uint32_t index);
void redbas_db_restore_cursor_set(RedbasDB *db, uint32_t index);
void redbas_db_restore(RedbasDB *db, void *data, uint32_t size);
void redbas_db_close(RedbasDB *db);

#endif
