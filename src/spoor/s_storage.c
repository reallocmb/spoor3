#include"spoor_internal.h"
#include"../redbas/redbas.h"

#include<stdio.h>
#include<string.h>
#include<dirent.h>

SpoorObject spoor_objects[500];
uint32_t spoor_objects_count;

void storage_db_path_clean(SpoorObject *spoor_object, char *db_path_clean)
{
    if (spoor_object->deadline.end.year == -1)
    {
        strcpy(db_path_clean, "000000");
    }
    else
    {
        uint32_t year = spoor_object->deadline.end.year + 1900;
        db_path_clean[3] = year % 10 + 0x30;
        year /= 10;
        db_path_clean[2] = year % 10 + 0x30;
        year /= 10;
        db_path_clean[1] = year % 10 + 0x30;
        year /= 10;
        db_path_clean[0] = year % 10 + 0x30;

        if (spoor_object->deadline.end.mon + 1 < 10)
        {
            db_path_clean[4] = '0';
            db_path_clean[5] = spoor_object->deadline.end.mon + 1 + 0x30;
        }
        else
        {
            db_path_clean[4] = (spoor_object->deadline.end.mon + 1) / 10 + 0x30;
            db_path_clean[5] = (spoor_object->deadline.end.mon + 1) % 10 + 0x30;
        }
    }

    db_path_clean[6] = 0;
}

void storage_db_path(SpoorObject *spoor_object, char *db_path)
{
    storage_db_path_clean(spoor_object, db_path);
    strcpy(db_path + 6, ".rdb");
    db_path[10] = 0;
}

uint32_t spoor_storage_link_space_free_index(RedbasDB *db)
{
    uint32_t items = redbas_db_items(db);
    SpoorLink spoor_object;
    uint32_t i;
    for (i = 0; i < items; i++)
    {
        redbas_db_restore_cursor_set(db, i);
        redbas_db_restore(db, &spoor_object, sizeof(spoor_object));

        if (spoor_object.id == SPOOR_OBJECT_DELETED_ID)
            break;
    }

    return i;
}

uint32_t spoor_storage_space_free_index(RedbasDB *db)
{
    uint32_t items = redbas_db_items(db);
    SpoorObject spoor_object;
    uint32_t i;
    for (i = 0; i < items; i++)
    {
        redbas_db_restore_cursor_set(db, i);
        redbas_db_restore(db, &spoor_object, sizeof(spoor_object));

        if (spoor_object.id == SPOOR_OBJECT_DELETED_ID)
            break;
    }

    return i;
}

void spoor_storage_save(SpoorObject *spoor_object)
{
    char location[11];
    RedbasDB *db_spoor_object;
    RedbasDB *db_spoor_object_parent;
    RedbasDB *db_links;

    SpoorObject *spoor_object_parent;

    storage_db_path(spoor_object, location);
    db_spoor_object = redbas_db_open(location, sizeof(*spoor_object));
    spoor_object->id = spoor_storage_space_free_index(db_spoor_object);

    if (spoor_object->id_link != SPOOR_OBJECT_NO_LINK) /* ip_link has the index of the parent object */
    {
        /*assign parent */
        spoor_object_parent = &spoor_objects[spoor_object->id_link];

        /* link_global set */
        storage_db_path(spoor_object, location);
        strcpy(link_global.location_child, location);
        link_global.id_child = spoor_object->id;

        storage_db_path(spoor_object_parent, location);
        strcpy(link_global.location_parent, location);
        link_global.id_parent = spoor_object_parent->id;

        db_links = redbas_db_open("links", sizeof(link_global));
        link_global.id = spoor_storage_link_space_free_index(db_links);

        /* check if spoor_object_parent has no link */
        if (spoor_object_parent->id_link == SPOOR_OBJECT_NO_LINK)
        {
            link_global.id_prev = SPOOR_OBJECT_NO_LINK;
            link_global.id_next = SPOOR_OBJECT_NO_LINK;
            spoor_object_parent->id_link = link_global.id;
        }
        else
        {
            SpoorLink link_parent_old;
            redbas_db_restore_cursor_set(db_links, spoor_object_parent->id_link);
            redbas_db_restore(db_links, &link_parent_old, sizeof(link_parent_old));
            link_parent_old.id_prev = link_global.id;
            redbas_db_change(db_links, &link_parent_old, sizeof(link_parent_old), link_parent_old.id);

            link_global.id_next = spoor_object_parent->id_link;
            spoor_object_parent->id_link = link_global.id;
        }

        /* link_global store */
        if (link_global.id == redbas_db_items(db_links))
            redbas_db_store(db_links, &link_global, sizeof(link_global));
        else
            redbas_db_change(db_links, &link_global, sizeof(link_global), link_global.id);


        redbas_db_close(db_links);

        /* spoor_object_parent store */
        storage_db_path(spoor_object_parent, location);
        db_spoor_object_parent = redbas_db_open(location, sizeof(*spoor_object_parent));
        redbas_db_change(db_spoor_object_parent,
                         spoor_object_parent,
                         sizeof(*spoor_object_parent),
                         spoor_object_parent->id);

        redbas_db_close(db_spoor_object_parent);


        /* reference spoor_object to links id */
        strcpy(spoor_object->parent_title, spoor_object_parent->title);
        spoor_object->id_link = link_global.id;
    }


    if (spoor_object->id == redbas_db_items(db_spoor_object))
        redbas_db_store(db_spoor_object, spoor_object, sizeof(*spoor_object));
    else
        redbas_db_change(db_spoor_object, spoor_object, sizeof(*spoor_object), spoor_object->id);
    redbas_db_close(db_spoor_object);

}

uint32_t spoor_object_storage_load(SpoorFilter *spoor_filter)
{
    DIR *dir = opendir(".");
    struct dirent *ptr;
    uint32_t i;
    uint32_t items_total = 0;
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strncmp(ptr->d_name + 6, ".rdb", 3) == 0)
        {
            RedbasDB *db = redbas_db_open(ptr->d_name, sizeof(*spoor_objects));
            uint32_t items = redbas_db_items(db);
            for (i = 0; i < items; i++, items_total++)
            {
                redbas_db_restore_cursor_set(db, i);
                redbas_db_restore(db, &spoor_objects[items_total], sizeof(*spoor_objects));
                /* skip temporly deleted elements */
                if (spoor_objects[items_total].id == SPOOR_OBJECT_DELETED_ID)
                    items_total--;

                /* filter */
#if 0
                printf("stc: %ld\n%ld\n", spoor_time_compare(&spoor_objects[items_total + i].deadline.end, &spoor_filter->spoor_time.start),
                        spoor_time_compare(&spoor_objects[items_total + i].deadline.end, &spoor_filter->spoor_time.end));
                if (spoor_time_compare(&spoor_objects[items_total + i].deadline.end, &spoor_filter->spoor_time.start) < 0 ||
                    spoor_time_compare(&spoor_objects[items_total + i].deadline.end, &spoor_filter->spoor_time.end) > 0)
                    items_total--;
#endif
            }

            redbas_db_close(db);
        }
    }

    closedir(dir);
    return items_total;
}

uint32_t spoor_object_storage_load_filter_time_span(SpoorTimeSpan *time_span)
{
    DIR *dir = opendir(".");
    struct dirent *ptr;
    uint32_t items_total = 0;
    uint32_t i;
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strncmp(ptr->d_name + 6, ".rdb", 3) == 0)
        {
            RedbasDB *db = redbas_db_open(ptr->d_name, sizeof(*spoor_objects));
            uint32_t items = redbas_db_items(db);
            for (i = 0; i < items; i++)
            {
                redbas_db_restore_cursor_set(db, i);
                redbas_db_restore(db, &spoor_objects[items_total + i], sizeof(*spoor_objects));
                /* skip temporly deleted elements */
                if (spoor_objects[items_total + i].id == SPOOR_OBJECT_DELETED_ID)
                    items_total--;

                /* time span filter */
                if (spoor_time_compare(&spoor_objects[items_total + i].deadline.end, &time_span->start) >= 0)
                    items_total--;
            }

            items_total += items;

            redbas_db_close(db);
        }
    }

    closedir(dir);
    spoor_sort_objects_by_deadline();
    return items_total;
}

void spoor_storage_change(SpoorObject *spoor_object_old, SpoorObject *spoor_object)
{
    char location_old[11];
    char location[11];
    storage_db_path(spoor_object_old, location_old);
    storage_db_path(spoor_object, location);

    spoor_storage_delete(spoor_object_old);
    spoor_storage_save(spoor_object);
#if 0
    /* if spoor_object has new location */
    if (strcmp(location_old, location) != 0)
    {
        spoor_storage_delete(spoor_object_old);
        spoor_storage_save(spoor_object);
    }
    else
    {
    }

    RedbasDB *db = redbas_db_open(location, sizeof(*spoor_object));
    redbas_db_change(db, spoor_object, sizeof(*spoor_object), spoor_object->id);
    redbas_db_close(db);
#endif
}

void spoor_storage_delete(SpoorObject *spoor_object)
{
    char db_path[11];
    storage_db_path(spoor_object, db_path);

    uint32_t spoor_object_id_backup = spoor_object->id;
    spoor_object->id = SPOOR_OBJECT_DELETED_ID;

    /* check if spoor_object has link */
    if (spoor_object->id_link != SPOOR_OBJECT_NO_LINK)
    {
        SpoorObject spoor_object_tmp;

        /* restore global_link */
        RedbasDB *db_links = redbas_db_open("links", sizeof(link_global));
        redbas_db_restore_cursor_set(db_links, spoor_object->id_link);
        redbas_db_restore(db_links, &link_global, sizeof(link_global));

        /* remove current global_link and save */
        uint32_t link_global_id_backup = link_global.id;
        link_global.id = SPOOR_OBJECT_DELETED_ID;
        redbas_db_change(db_links, &link_global, sizeof(link_global), link_global_id_backup);

        /* check if spoor_object link is parent */
        char location[11];
        storage_db_path(spoor_object, location);
        if (spoor_object_id_backup == link_global.id_parent &&
            strcmp(location, link_global.location_parent) == 0)
        {
            /* remove link from spoor_object child */
            RedbasDB *db_spoor_object = redbas_db_open(link_global.location_child, sizeof(link_global));
            redbas_db_restore_cursor_set(db_spoor_object, link_global.id_child);
            redbas_db_restore(db_spoor_object, &spoor_object_tmp, sizeof(spoor_object_tmp));
            spoor_object_tmp.id_link = SPOOR_OBJECT_NO_LINK;
            spoor_object_tmp.parent_title[0] = '-';
            spoor_object_tmp.parent_title[1] = 0;
            redbas_db_change(db_spoor_object, &spoor_object_tmp, sizeof(spoor_object_tmp), spoor_object_tmp.id);
            redbas_db_close(db_spoor_object);

            /* links all childs delete */
            while (link_global.id_next != SPOOR_OBJECT_NO_LINK)
            {
                redbas_db_restore_cursor_set(db_links, link_global.id_next);
                redbas_db_restore(db_links, &link_global, sizeof(link_global));
                link_global_id_backup = link_global.id;
                link_global.id = SPOOR_OBJECT_DELETED_ID;
                redbas_db_change(db_links, &link_global, sizeof(link_global), link_global_id_backup);

                /* remove link from spoor_object child */
                RedbasDB *db_spoor_object = redbas_db_open(link_global.location_child, sizeof(link_global));
                redbas_db_restore_cursor_set(db_spoor_object, link_global.id_child);
                redbas_db_restore(db_spoor_object, &spoor_object_tmp, sizeof(spoor_object_tmp));
                spoor_object_tmp.id_link = SPOOR_OBJECT_NO_LINK;
                spoor_object_tmp.parent_title[0] = '-';
                spoor_object_tmp.parent_title[1] = 0;
                redbas_db_change(db_spoor_object, &spoor_object_tmp, sizeof(spoor_object_tmp), spoor_object_tmp.id);
                redbas_db_close(db_spoor_object);
            }
        }
        else /* if spoor_object is child */
        {
            /* when first link is removed */
            if (link_global.id_prev == SPOOR_OBJECT_NO_LINK)
            {
                /* remove link from parenat and set it to next child */
                RedbasDB *db_spoor_object = redbas_db_open(link_global.location_parent, sizeof(link_global));
                redbas_db_restore_cursor_set(db_spoor_object, link_global.id_parent);
                redbas_db_restore(db_spoor_object, &spoor_object_tmp, sizeof(spoor_object_tmp));

                /* when only one link child */
                if (link_global.id_next == SPOOR_OBJECT_NO_LINK)
                    spoor_object_tmp.id_link = SPOOR_OBJECT_NO_LINK;
                else
                {
                    spoor_object_tmp.id_link = link_global.id_next;
                    /* get next child */
                    redbas_db_restore_cursor_set(db_links, link_global.id_next);
                    redbas_db_restore(db_links, &link_global, sizeof(link_global));
                    link_global.id_prev = SPOOR_OBJECT_NO_LINK;
                    redbas_db_change(db_links, &link_global, sizeof(link_global), link_global.id);
                }

                redbas_db_change(db_spoor_object, &spoor_object_tmp, sizeof(spoor_object_tmp), spoor_object_tmp.id);
                redbas_db_close(db_spoor_object);
            }
            else if (link_global.id_next != SPOOR_OBJECT_NO_LINK) /* when a middle link is removed */
            {
                SpoorLink link_prev;
                SpoorLink link_next;
                redbas_db_restore_cursor_set(db_links, link_global.id_prev);
                redbas_db_restore(db_links, &link_prev, sizeof(link_prev));
                redbas_db_restore_cursor_set(db_links, link_global.id_next);
                redbas_db_restore(db_links, &link_next, sizeof(link_next));

                link_prev.id_next = link_next.id;
                link_next.id_prev = link_prev.id;
                redbas_db_change(db_links, &link_prev, sizeof(link_prev), link_prev.id);
                redbas_db_change(db_links, &link_next, sizeof(link_next), link_next.id);
            }
            else /* when the last link is removed */
            {
                /* get prev child */
                redbas_db_restore_cursor_set(db_links, link_global.id_prev);
                redbas_db_restore(db_links, &link_global, sizeof(link_global));

                link_global.id_next = SPOOR_OBJECT_NO_LINK;
                redbas_db_change(db_links, &link_global, sizeof(link_global), link_global.id);
            }
        }

        redbas_db_close(db_links);
    }

    storage_db_path(spoor_object, db_path);
    RedbasDB *db = redbas_db_open(db_path, sizeof(*spoor_object));
    redbas_db_change(db, spoor_object, sizeof(*spoor_object), spoor_object_id_backup);
    redbas_db_close(db);
}

#if 0
void spoor_storage_clean_up(void)
{
    DIR *dir = opendir(".");
    struct dirent *ptr;
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strncmp(ptr->d_name + 6, ".rdb", 3) == 0)
        {
            SpoorObject spoor_object;
            RedbasDB *db = redbas_db_open(ptr->d_name, sizeof(spoor_object));
            RedbasDB *db_tmp = redbas_db_open(".tmp_db", sizeof(spoor_object));

            uint32_t items = redbas_db_items(db);
            uint32_t i;
            uint32_t spoor_object_index;
            for (i = 0, spoor_object_index = 0; i < items; i++)
            {
                redbas_db_restore_cursor_set(db, i);
                redbas_db_restore(db, &spoor_object, sizeof(spoor_object));
                if (spoor_object.id != SPOOR_OBJECT_DELETED_ID)
                {
                    spoor_object.id = spoor_object_index;
                    redbas_db_store(db_tmp, &spoor_object, sizeof(spoor_object));
                    spoor_object_index++;
                }
           }

            redbas_db_close(db_tmp);
            redbas_db_close(db);
            rename(".tmp_db", ptr->d_name);
        }
    }

    closedir(dir);
}
#endif
