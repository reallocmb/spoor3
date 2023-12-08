#include"spoor_internal.h"
#include"../redbas/redbas.h"

SpoorLink link_global = { 0xffffffff, "", 0xffffffff, "", 0xffffffff, SPOOR_OBJECT_NO_LINK, SPOOR_OBJECT_NO_LINK };

void spoor_link_load(uint32_t id)
{
    RedbasDB *db = redbas_db_open("links", sizeof(link_global));
    redbas_db_restore_cursor_set(db, id);
    redbas_db_restore(db, &link_global, sizeof(link_global));
    redbas_db_close(db);
}
