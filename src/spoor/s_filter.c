#include"spoor_internal.h"

u32 spoor_filter_use(u32 *spoor_objects_indexes_orginal, SpoorFilter *spoor_filter)
{
    u32 i, j;
    for (i = 0, j = 0; i < spoor_objects_count; i++)
    {
        if (!(spoor_filter->types & TYPE_BITS[spoor_objects[i].type]))
            continue;
        if (!(spoor_filter->status & STATUS_BITS[spoor_objects[i].status]))
            continue;
        spoor_objects_indexes_orginal[j] = i;
        j++;
    }

    return j;
}

void spoor_filter_change(SpoorFilter *spoor_filter, char *arguments)
{
    /* first charachter is ' ' */
    if (arguments[0] == ' ')
        arguments++;

    u32 argument_length = 0;
    while ((argument_length = arguments_next(&arguments, argument_length)) != 0)
    {
        if (arguments[0] == 't')
        {
            bool minus = false;

            u32 i;
            for (i = 1; i < argument_length; i++)
            {
                if (minus)
                {
                    switch (arguments[i])
                    {
                        case 't': spoor_filter->types &= ~FILTER_TYPE_TASK; break;
                        case 'p': spoor_filter->types &= ~FILTER_TYPE_PROJECT; break;
                        case 'e': spoor_filter->types &= ~FILTER_TYPE_EVENT; break;
                        case 'a': spoor_filter->types &= ~FILTER_TYPE_APPOINTMENT; break;
                        case 'g': spoor_filter->types &= ~FILTER_TYPE_GOAL; break;
                        case 'h': spoor_filter->types &= ~FILTER_TYPE_HABIT; break;
                        case '+': minus = false; break;
                        case '.': spoor_filter->types = FILTER_TYPE_ALL; break;
                        case '0': spoor_filter->types = 0; break;
                    }
                }
                else
                {
                    switch (arguments[i])
                    {
                        case 't': spoor_filter->types |= FILTER_TYPE_TASK; break;
                        case 'p': spoor_filter->types |= FILTER_TYPE_PROJECT; break;
                        case 'e': spoor_filter->types |= FILTER_TYPE_EVENT; break;
                        case 'a': spoor_filter->types |= FILTER_TYPE_APPOINTMENT; break;
                        case 'g': spoor_filter->types |= FILTER_TYPE_GOAL; break;
                        case 'h': spoor_filter->types |= FILTER_TYPE_HABIT; break;
                        case '-': minus = true; break;
                        case '.': spoor_filter->types = FILTER_TYPE_ALL; break;
                        case '0': spoor_filter->types = 0; break;
                    }
                }
            }
        }
        else if (arguments[0] == 's')
        {
            spoor_filter->status = 0;
            if (argument_length == 4 && strncmp(arguments + 1, "all", 3) == 0)
                spoor_filter->status = FILTER_STATUS_ALL;
            else
            {
                u32 i;
                for (i = 1; i < argument_length; i++)
                {
                    switch (arguments[i])
                    {
                        case 'n': spoor_filter->status |= FILTER_STATUS_NOT_STARTED; break;
                        case 'i': spoor_filter->status |= FILTER_STATUS_IN_PROGRESS; break;
                        case 'c': spoor_filter->status |= FILTER_STATUS_COMPLETED; break;
                    }
                }
            }
        }
    }
}

void spoor_filter_buffer_create(SpoorFilter *spoor_filter, char *buffer26)
{
    sprintf(buffer26, "FILTER TYPE FLAGS: %s%s%s%s%s%s",
            (spoor_filter->types & FILTER_TYPE_TASK) ?"T" :"0",
            (spoor_filter->types & FILTER_TYPE_PROJECT) ?"P" :"0",
            (spoor_filter->types & FILTER_TYPE_EVENT) ?"E" :"0",
            (spoor_filter->types & FILTER_TYPE_APPOINTMENT) ?"A" :"0",
            (spoor_filter->types & FILTER_TYPE_GOAL) ?"G" :"0",
            (spoor_filter->types & FILTER_TYPE_HABIT) ?"H" :"0");
}
