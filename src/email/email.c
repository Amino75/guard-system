#include "email/email.h"
#include "common/config.h"

#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CURL *curl = NULL;

static const char *smtp_host = NULL;
static const char *smtp_username = NULL;
static const char *smtp_password = NULL;
static const char *smtp_recipient = NULL;

static int smtp_port = 587;
static int email_enabled = 0;


/*
 * Initialize the email subsystem.
 *
 * Gmail:
 *
 * smtp.gmail.com:587
 * STARTTLS
 */
int email_init(void)
{
    CURLcode result;

    email_enabled =
        config_get_bool("email.enabled");

    if (!email_enabled)
    {
        printf(
            "Email subsystem disabled\n"
        );

        return 0;
    }

    smtp_host =
        config_get_string("email.smtp_host");

    smtp_username =
        config_get_string("email.username");

    smtp_password =
        config_get_string("email.password");

    smtp_recipient =
        config_get_string("email.recipient");

    smtp_port =
        config_get_int("email.smtp_port");

    if (smtp_port <= 0)
        smtp_port = 587;

    if (smtp_host == NULL ||
        smtp_username == NULL ||
        smtp_password == NULL ||
        smtp_recipient == NULL)
    {
        fprintf(
            stderr,
            "Email: incomplete SMTP configuration\n"
        );

        return -1;
    }

    result =
        curl_global_init(
            CURL_GLOBAL_DEFAULT
        );

    if (result != CURLE_OK)
    {
        fprintf(
            stderr,
            "Email: curl_global_init failed: %s\n",
            curl_easy_strerror(result)
        );

        return -1;
    }

    curl =
        curl_easy_init();

    if (curl == NULL)
    {
        fprintf(
            stderr,
            "Email: curl_easy_init failed\n"
        );

        curl_global_cleanup();

        return -1;
    }

    printf(
        "Email subsystem initialized\n"
    );

    printf(
        "Email SMTP server: %s:%d\n",
        smtp_host,
        smtp_port
    );

    return 0;
}


/*
 * Send one guard-system detection alert.
 *
 * The JPEG is already encoded in memory.
 *
 * person_count:
 *     Number of detected persons.
 *
 * timestamp:
 *     Detection timestamp.
 *
 * cpu_temperature:
 *     Current CPU temperature.
 *
 * jpeg_data:
 *     JPEG image corresponding to the detection.
 *
 * jpeg_size:
 *     Size of JPEG buffer.
 */
int email_send_alert(
    unsigned int person_count,
    const char *timestamp,
    double cpu_temperature,
    const unsigned char *jpeg_data,
    size_t jpeg_size
)
{
    CURLcode result;

    curl_mime *mime = NULL;
    curl_mimepart *part = NULL;

    char body[1024];

    char subject[128];

    struct curl_slist *recipients = NULL;
struct curl_slist *headers = NULL;

    if (!email_enabled)
        return 0;

    if (curl == NULL)
    {
        fprintf(
            stderr,
            "Email: CURL handle not initialized\n"
        );

        return -1;
    }

    if (timestamp == NULL)
    {
        timestamp =
            "unknown";
    }

    if (jpeg_data == NULL ||
        jpeg_size == 0)
    {
        fprintf(
            stderr,
            "Email: invalid JPEG data\n"
        );

        return -1;
    }


    /*
     * Subject.
     */

    snprintf(
        subject,
        sizeof(subject),
        "Guard System Alert - %u person%s detected",
        person_count,
        person_count == 1 ? "" : "s"
    );


    /*
     * Email text body.
     */

    snprintf(
        body,
        sizeof(body),

        "Guard System Alert\n"
        "\n"
        "Person(s) detected: %u\n"
        "Timestamp: %s\n"
        "CPU temperature: %.2f C\n"
        "\n"
        "The image captured at the time of detection "
        "is attached to this email.\n",

        person_count,
        timestamp,
        cpu_temperature
    );


    /*
     * Reset CURL options from any previous request.
     */

    curl_easy_reset(curl);


    /*
     * Gmail SMTP server.
     *
     * Port 587 uses STARTTLS.
     */

    char smtp_url[256];

    snprintf(
        smtp_url,
        sizeof(smtp_url),
        "smtp://%s:%d",
        smtp_host,
        smtp_port
    );

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        smtp_url
    );


    /*
     * Gmail authentication.
     *
     * The password is the Gmail App Password.
     */

    curl_easy_setopt(
        curl,
        CURLOPT_USERNAME,
        smtp_username
    );

    curl_easy_setopt(
        curl,
        CURLOPT_PASSWORD,
        smtp_password
    );


    /*
     * Enable STARTTLS.
     */

    curl_easy_setopt(
        curl,
        CURLOPT_USE_SSL,
        (long)CURLUSESSL_ALL
    );


    /*
     * Sender.
     */

    curl_easy_setopt(
        curl,
        CURLOPT_MAIL_FROM,
        smtp_username
    );


    /*
     * Recipient.
     */

    recipients =
        curl_slist_append(
            NULL,
            smtp_recipient
        );

    if (recipients == NULL)
    {
        fprintf(
            stderr,
            "Email: failed to create recipient list\n"
        );

        return -1;
    }

    curl_easy_setopt(
        curl,
        CURLOPT_MAIL_RCPT,
        recipients
    );


    /*
     * Create MIME message.
     */

    mime =
        curl_mime_init(curl);

    if (mime == NULL)
    {
        fprintf(
            stderr,
            "Email: curl_mime_init failed\n"
        );

        curl_slist_free_all(
            recipients
        );

        return -1;
    }


    /*
     * MIME part 1:
     *
     * Plain text email body.
     */

    part =
        curl_mime_addpart(mime);

    if (part == NULL)
    {
        fprintf(
            stderr,
            "Email: failed to create text MIME part\n"
        );

        curl_mime_free(mime);

        curl_slist_free_all(
            recipients
        );

        return -1;
    }

    curl_mime_data(
        part,
        body,
        CURL_ZERO_TERMINATED
    );

    curl_mime_type(
        part,
        "text/plain"
    );


    /*
     * MIME part 2:
     *
     * JPEG detection image.
     *
     * The JPEG remains in memory.
     * libcurl copies the supplied data
     * into the MIME part.
     */

    part =
        curl_mime_addpart(mime);

    if (part == NULL)
    {
        fprintf(
            stderr,
            "Email: failed to create JPEG MIME part\n"
        );

        curl_mime_free(mime);

        curl_slist_free_all(
            recipients
        );

        return -1;
    }

    curl_mime_data(
        part,
        (const char *)jpeg_data,
        jpeg_size
    );

    curl_mime_filename(
        part,
        "detection.jpg"
    );

    curl_mime_type(
        part,
        "image/jpeg"
    );

    curl_mime_encoder(
        part,
        "base64"
    );


    /*
     * Set MIME message.
     */

    curl_easy_setopt(
        curl,
        CURLOPT_MIMEPOST,
        mime
    );


/*
 * Email headers.
 *
 * libcurl uses CURLOPT_HTTPHEADER for the
 * top-level RFC 5322 headers of SMTP MIME mail.
 */



char subject_header[256];
char from_header[512];
char to_header[512];

snprintf(
    subject_header,
    sizeof(subject_header),
    "Subject: %s",
    subject
);

snprintf(
    from_header,
    sizeof(from_header),
    "From: %s",
    smtp_username
);

snprintf(
    to_header,
    sizeof(to_header),
    "To: %s",
    smtp_recipient
);

headers =
    curl_slist_append(
        headers,
        from_header
    );

headers =
    curl_slist_append(
        headers,
        to_header
    );

headers =
    curl_slist_append(
        headers,
        subject_header
    );

headers =
    curl_slist_append(
        headers,
        "MIME-Version: 1.0"
    );

curl_easy_setopt(
    curl,
    CURLOPT_HTTPHEADER,
    headers
);


    /*
     * Keep the SMTP operation quiet.
     *
     * Application logging handles errors.
     */

    curl_easy_setopt(
        curl,
        CURLOPT_VERBOSE,
        0L
    );


    /*
     * Send email.
     */

    printf(
        "Email: sending alert "
        "(persons=%u, temperature=%.2f C)\n",
        person_count,
        cpu_temperature
    );

    result =
        curl_easy_perform(curl);


    /*
     * Release MIME resources.
     */

    curl_mime_free(mime);

    curl_slist_free_all(
        recipients
    );
curl_slist_free_all(headers);

    /*
     * Check result.
     */

    if (result != CURLE_OK)
    {
        fprintf(
            stderr,
            "Email: send failed: %s\n",
            curl_easy_strerror(result)
        );

        return -1;
    }


    printf(
        "Email: alert sent successfully\n"
    );

    return 0;
}


/*
 * Shutdown email subsystem.
 */
void email_cleanup(void)
{
    if (curl != NULL)
    {
        curl_easy_cleanup(curl);

        curl = NULL;
    }

    if (email_enabled)
    {
        curl_global_cleanup();
    }

    printf(
        "Email subsystem cleaned up\n"
    );
}
