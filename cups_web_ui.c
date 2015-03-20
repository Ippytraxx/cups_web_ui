/*
 * No frills web ui for printing files on a CUPS server
 *
 * Deps: libcups, libmongoose
 *
 * Copyright (c) 2015 Vincent Szolnoky <vincent@szolnoky.com>
 */

#include <stdio.h>
#include <string.h>
#include <mongoose.h>
#include <cups/cups.h>

#define PORT "8080"
#define FILE_TEMPLATE "/tmp/cups_web_ui_XXXXXX"

static int send_index_page(struct mg_connection* conn)
{
    char name[] = FILE_TEMPLATE;
    int data_len = 0, ofs = 0;
    const char* data;
    int fd;
    FILE* file;
    char var_name[100], file_name[100];
    int has_file = 0;
    cups_dest_t* dests;
    cups_dest_t* dest;
    char* dest_name;
    int i;

    int num_dests = cupsGetDests(&dests);

    mg_printf_data(conn, "%s", "<html><body><form method=\"post\" name=\"print\" enctype=\"multipart/form-data\">"
                   "<div><label for=\"printer\">Choose printer: </label><select name=\"printer\"></div>");
    for(i = 0, dest = dests; i < num_dests; i++, dest++)
    {
        mg_printf_data(conn, "<option value=\"%s\">%s</option>", dest->name, dest->name);
    }
    mg_printf_data(conn, "%s", "</select>"
                   "<div>Choose file: <input type=\"file\" accept=\"application/pdf\" name=\"file\"/></div>"
                   "<div><input type=\"submit\" value=\"Print\"/></form></div>");


    fd = mkstemp(name);
    file = fdopen(fd, "w+");

    while ((ofs = mg_parse_multipart(conn->content + ofs, conn->content_len - ofs, var_name, 
                                     sizeof(var_name), file_name, sizeof(file_name), &data, &data_len)) > 0)
    {
        printf("%s\n", var_name);
        if (strcmp(var_name, "file") == 0)
        {
            has_file = 1;
            fwrite(data, sizeof(char), data_len/sizeof(char), file);
        }
        else if (strcmp(var_name, "printer") == 0)
            dest_name = strndup(data, data_len);
    }

    fclose(file);
    close(fd);

    if(has_file)
    {
        printf("Print %s to %s\n", dest_name, name);
        cupsPrintFile(dest_name, name, file_name, 0, NULL);
        mg_printf_data(conn, "<p>File <i>%s</i> accepted for printing.</p>", file_name);
    }

    unlink(name);

    mg_printf_data(conn, "%s", "</body></html>");

    return MG_TRUE;
}

static int evt_handler(struct mg_connection* conn, enum mg_event evt)
{
    switch(evt)
    {
        case MG_AUTH:       return MG_TRUE;
        case MG_REQUEST:    return send_index_page(conn);
        default:            return MG_FALSE;
    }
}

int main()
{
    struct mg_server* server;

    server = mg_create_server(NULL, evt_handler);
    mg_set_option(server, "listening_port", PORT);

    while (1)
    {
        mg_poll_server(server, 1000);
    }

    mg_destroy_server(&server);

    return 0;
}
