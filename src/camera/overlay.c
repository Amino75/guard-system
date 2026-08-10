#include "camera/overlay.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

/*
 * YUYV format:
 *
 * Y0 U0 Y1 V0
 *
 * One U/V pair is shared by two horizontal pixels.
 *
 * For drawing, we modify the Y component only.
 * This produces a bright/dark monochrome overlay
 * without disturbing the chroma information.
 */

static void set_pixel(
    unsigned char *yuyv,
    int width,
    int height,
    int x,
    int y,
    unsigned char value
)
{
    if (y < 0 || y >= height ||
        x < 0 || x >= width)
    {
        return;
    }

    size_t index =
        ((size_t)y * width + x) * 2;

    yuyv[index] = value;
}


/*
 * Draw a horizontal line.
 */
static void draw_horizontal(
    unsigned char *yuyv,
    int width,
    int height,
    int x1,
    int x2,
    int y
)
{
    if (y < 0 || y >= height)
        return;

    if (x1 > x2)
    {
        int tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    if (x1 < 0)
        x1 = 0;

    if (x2 >= width)
        x2 = width - 1;

    for (int x = x1; x <= x2; x++)
    {
        set_pixel(
            yuyv,
            width,
            height,
            x,
            y,
            235
        );
    }
}


/*
 * Draw a vertical line.
 */
static void draw_vertical(
    unsigned char *yuyv,
    int width,
    int height,
    int x,
    int y1,
    int y2
)
{
    if (x < 0 || x >= width)
        return;

    if (y1 > y2)
    {
        int tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    if (y1 < 0)
        y1 = 0;

    if (y2 >= height)
        y2 = height - 1;

    for (int y = y1; y <= y2; y++)
    {
        set_pixel(
            yuyv,
            width,
            height,
            x,
            y,
            50
        );
    }
}


/*
 * Draw a rectangular bounding box.
 */
static void draw_rectangle(
    unsigned char *yuyv,
    int width,
    int height,
    int x,
    int y,
    int box_width,
    int box_height
)
{
    int x2 = x + box_width - 1;
    int y2 = y + box_height - 1;

    /*
     * Make the box visible even when the
     * detector produces a very small box.
     */
    draw_horizontal(
        yuyv,
        width,
        height,
        x,
        x2,
        y
    );

    draw_horizontal(
        yuyv,
        width,
        height,
        x,
        x2,
        y2
    );

    draw_vertical(
        yuyv,
        width,
        height,
        x,
        y,
        y2
    );

    draw_vertical(
        yuyv,
        width,
        height,
        x2,
        y,
        y2
    );
}


/*
 * Tiny 5x7 bitmap font.
 *
 * Characters required by the overlay:
 *
 * P E R S O N
 * # digits
 * . %
 * space
 */
static const unsigned char font_digits[10][7] =
{
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, /* 0 */
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, /* 1 */
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, /* 2 */
    {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}, /* 3 */
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, /* 4 */
    {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}, /* 5 */
    {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}, /* 6 */
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, /* 7 */
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, /* 8 */
    {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}  /* 9 */
};


static const unsigned char font_P[7] =
{
    0x1E,
    0x11,
    0x11,
    0x1E,
    0x10,
    0x10,
    0x10
};


static const unsigned char font_E[7] =
{
    0x1F,
    0x10,
    0x10,
    0x1E,
    0x10,
    0x10,
    0x1F
};


static const unsigned char font_R[7] =
{
    0x1E,
    0x11,
    0x11,
    0x1E,
    0x14,
    0x12,
    0x11
};


static const unsigned char font_S[7] =
{
    0x0F,
    0x10,
    0x10,
    0x0E,
    0x01,
    0x01,
    0x1E
};


static const unsigned char font_O[7] =
{
    0x0E,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x0E
};


static const unsigned char font_N[7] =
{
    0x11,
    0x19,
    0x15,
    0x13,
    0x11,
    0x11,
    0x11
};


static const unsigned char font_HASH[7] =
{
    0x0A,
    0x1F,
    0x0A,
    0x0A,
    0x1F,
    0x0A,
    0x00
};


static const unsigned char font_PERCENT[7] =
{
    0x19,
    0x19,
    0x02,
    0x04,
    0x08,
    0x13,
    0x13
};


static const unsigned char font_DOT[7] =
{
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x0C
};



static const unsigned char font_I[7] =
{
    0x1F,
    0x04,
    0x04,
    0x04,
    0x04,
    0x04,
    0x1F
};

static const unsigned char font_D[7] =
{
    0x1E,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x1E
};

static const unsigned char font_A[7] =
{
    0x0E,
    0x11,
    0x11,
    0x1F,
    0x11,
    0x11,
    0x11
};

static const unsigned char font_T[7] =
{
    0x1F,
    0x04,
    0x04,
    0x04,
    0x04,
    0x04,
    0x04
};

static const unsigned char font_M[7] =
{
    0x11,
    0x1B,
    0x15,
    0x15,
    0x11,
    0x11,
    0x11
};

static const unsigned char font_C[7] =
{
    0x0E,
    0x11,
    0x10,
    0x10,
    0x10,
    0x11,
    0x0E
};

static const unsigned char font_COLON[7] =
{
    0x00,
    0x0C,
    0x0C,
    0x00,
    0x0C,
    0x0C,
    0x00
};

static const unsigned char *get_glyph(char c)
{
    if (c >= '0' && c <= '9')
        return font_digits[c - '0'];

    switch (c)
    {
        case 'P': return font_P;
        case 'E': return font_E;
        case 'R': return font_R;
        case 'S': return font_S;
        case 'O': return font_O;
        case 'N': return font_N;
    case 'I': return font_I;
    case 'D': return font_D;
    case 'A': return font_A;
    case 'T': return font_T;
    case 'M': return font_M;
    case ':': return font_COLON;
        case '#': return font_HASH;
        case '%': return font_PERCENT;
        case '.': return font_DOT;
        default:
            return NULL;
    }
}


static void draw_char(
    unsigned char *yuyv,
    int width,
    int height,
    int x,
    int y,
    char c,
    int scale
)
{
    const unsigned char *glyph =
        get_glyph(c);

    if (glyph == NULL)
        return;

    for (int row = 0; row < 7; row++)
    {
        for (int col = 0; col < 5; col++)
        {
            if (glyph[row] &
                (1 << (4 - col)))
            {
                for (int sy = 0; sy < scale; sy++)
                {
                    for (int sx = 0; sx < scale; sx++)
                    {
                        set_pixel(
                            yuyv,
                            width,
                            height,
                            x + col * scale + sx,
                            y + row * scale + sy,
                            50
                        );
                    }
                }
            }
        }
    }
}

static void get_live_datetime(
    char *date,
    size_t date_size,
    char *time_text,
    size_t time_size
)
{
    time_t now;
    struct tm local_time;

    time(&now);

    if (localtime_r(&now, &local_time) == NULL)
    {
        snprintf(date, date_size, "0000-00-00");
        snprintf(time_text, time_size, "00:00:00");
        return;
    }

    strftime(
        date,
        date_size,
        "%Y-%m-%d",
        &local_time
    );

    strftime(
        time_text,
        time_size,
        "%H:%M:%S",
        &local_time
    );
}


static void draw_text(
    unsigned char *yuyv,
    int width,
    int height,
    int x,
    int y,
    const char *text,
    int scale
)
{
    int cursor_x = x;

    while (*text != '\0')
    {
        draw_char(
            yuyv,
            width,
            height,
            cursor_x,
            y,
            *text,
            scale
        );

        cursor_x += 6 * scale;

        text++;
    }
}


/*
 * Draw all detections.
 *
 * Each detection contains:
 *
 * x
 * y
 * width
 * height
 * confidence
 */
void overlay_draw_detections(
    unsigned char *yuyv,
    int width,
    int height,
    const detection_box_t *detections,
    uint32_t count,
    const char *student_id,
    double stream_fps
)
{

    char live_date[16];
    char live_time[16];

    get_live_datetime(
        live_date,
        sizeof(live_date),
        live_time,
        sizeof(live_time)
    );

    if (student_id == NULL)
        student_id = "404211106";

char student_text[32];
char date_text[32];
char time_text_local[32];
char fps_text[32];


snprintf(
    student_text,
    sizeof(student_text),
    "ID:%s",
    student_id
);

snprintf(
    date_text,
    sizeof(date_text),
    "DATE:%s",
    live_date
);

snprintf(
    time_text_local,
    sizeof(time_text_local),
    "TIME:%s",
    live_time
);

snprintf(
    fps_text,
    sizeof(fps_text),
    "FPS:%.2f",
    stream_fps
);


    if (yuyv == NULL ||
        detections == NULL)
    {
        return;
    }

    if (count > MAX_DETECTIONS)
        count = MAX_DETECTIONS;

/*
 * Draw system identification and live time.
 */


draw_text(
    yuyv,
    width,
    height,
    10,
    10,
    student_text,
    1
);

draw_text(
    yuyv,
    width,
    height,
    10,
    20,
    date_text,
    1
);

draw_text(
    yuyv,
    width,
    height,
    10,
    30,
    time_text_local,
    1
);
draw_text(
    yuyv,
    width,
    height,
    10,
    40,
    fps_text,
    1
);

    for (uint32_t i = 0; i < count; i++)
    {
        int x =
            detections[i].x;

        int y =
            detections[i].y;

        int box_width =
            detections[i].width;

        int box_height =
            detections[i].height;

        if (box_width <= 0 ||
            box_height <= 0)
        {
            continue;
        }

        /*
         * Clamp bounding box to camera frame.
         */
        if (x < 0)
        {
            box_width += x;
            x = 0;
        }

        if (y < 0)
        {
            box_height += y;
            y = 0;
        }

        if (x + box_width > width)
            box_width = width - x;

        if (y + box_height > height)
            box_height = height - y;

        if (box_width <= 0 ||
            box_height <= 0)
        {
            continue;
        }

        /*
         * Draw bounding box.
         */
        draw_rectangle(
            yuyv,
            width,
            height,
            x,
            y,
            box_width,
            box_height
        );

        /*
         * Person number.
         *
         * Example:
         *
         * PERSON #1
         */
        char label[32];

        snprintf(
            label,
            sizeof(label),
            "PERSON#%u",
            i + 1
        );

        /*
         * Confidence is converted to
         * percentage.
         *
         * Example:
         *
         * 87%
         */
        int confidence =
            (int)(
                detections[i].confidence * 100.0f
                + 0.5f
            );

        if (confidence < 0)
            confidence = 0;

        if (confidence > 100)
            confidence = 100;

        char confidence_text[16];

        snprintf(
            confidence_text,
            sizeof(confidence_text),
            "%d%%",
            confidence
        );

        /*
         * Draw labels above the bounding box
         * when possible.
         */
        int label_y =
            y - 18;

        if (label_y < 0)
            label_y = y + 2;

        draw_text(
            yuyv,
            width,
            height,
            x,
            label_y,
            label,
            2
        );

        draw_text(
            yuyv,
            width,
            height,
            x,
            label_y + 16,
            confidence_text,
            2
        );
/*
draw_text(
    yuyv,
    width,
    height,
    10,
    10,
    student_text,
    1
);

draw_text(
    yuyv,
    width,
    height,
    10,
    20,
    date_text,
    1
);

draw_text(
    yuyv,
    width,
    height,
    10,
    30,
    time_text_local,
    1
);
*/

    }
}
