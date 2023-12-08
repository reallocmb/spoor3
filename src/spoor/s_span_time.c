#include"spoor_internal.h"

#include<string.h>

void spoor_time_span_create(SpoorTimeSpan *spoor_time_span, char *command)
{
    /* command */
    /* d-1w means today until next week */

    int count = 0;
    int mode = 0;
    int sign = 1;


    uint32_t i = 0;

    if (command[0] == '-')
    {
        sign = -1;
        i++;
    }

    for (; i < strlen(command); i++)
    {
        if (command[i] >= 0x30 && command[i] <= 0x39)
        {
            count *= 10;
            count += command[i] - 0x30;
        }
        else
            break;
    }

    /* mode */
    mode = command[i];
    i++;

    /* start time  */
    time_t time_current;
    time_current = time(NULL);

    if (mode == 'd')
        time_current += count * 60 * 60 * 24 * sign;

    spoor_time_span->start = *((SpoorTime *)localtime(&time_current));

    if (command[i] == '-')
    {
        i++;
        if (command[i] == '-')
        {
            sign = -1;
            i++;
        }
        else
            sign = 1;

        for (; i < strlen(command); i++)
        {
            if (command[i] >= 0x30 && command[i] <= 0x39)
            {
                count *= 10;
                count += command[i] - 0x30;
            }
            else
                break;
        }

        /* mode */
        mode = command[i];
        i++;

        time_current = time(NULL);
        if (mode == 'd')
            time_current += count * 60 * 60 * 24 * sign;

        spoor_time_span->end = *(SpoorTime *)localtime(&time_current);
    }
    else
        spoor_time_span->end.year = -1;
}
