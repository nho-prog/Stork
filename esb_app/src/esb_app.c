/*
 * Copyright (c) 2016-2018 Joris Vink <joris@coders.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This example demonstrates how to properly deal with file uploads
 * coming from a multipart form.
 *
 * The basics are quite trivial:
 *	1) call http_populate_multipart_form()
 *	2) find the file using http_file_lookup().
 *	3) read the file data using http_file_read().
 *
 * In this example the contents is written to a newly created file
 * on the server that matches the naming given by the uploader.
 *
 * Note that the above is probably not what you want to do in real life.
 */

#include <kore/kore.h>
#include <kore/http.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "esb.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/un.h>
#include <stddef.h>

#define PATH_MAX 500

void log_msg(const char *msg, bool terminate) {
    printf("%s\n", msg);
    if (terminate) exit(-1); /* failure */
}

int make_named_socket(const char *socket_file, bool is_client) {
    printf("Creating AF_LOCAL socket at path %s\n", socket_file);
    if (!is_client && access(socket_file, F_OK) != -1) {
        log_msg("An old socket file exists, removing it.", false);
        if (unlink(socket_file) != 0) {
            log_msg("Failed to remove the existing socket file.", true);
        }
    }
    struct sockaddr_un name;
    /* Create the socket. */
    int sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        log_msg("Failed to create socket.", true);
    }

    /* Bind a name to the socket. */
    name.sun_family = AF_LOCAL;
    strncpy (name.sun_path, socket_file, sizeof(name.sun_path));
    name.sun_path[sizeof(name.sun_path) - 1] = '\0';

    /* The size of the address is
       the offset of the start of the socket_file,
       plus its length (not including the terminating null byte).
       Alternatively you can just do:
       size = SUN_LEN (&name);
   */
    size_t size = (offsetof(struct sockaddr_un, sun_path) +
                   strlen(name.sun_path));
    if (is_client) {
        if (connect(sock_fd, (struct sockaddr *) &name, size) < 0) {
            log_msg("connect failed", 1);
        }
    } else {
        if (bind(sock_fd, (struct sockaddr *) &name, size) < 0) {
            log_msg("bind failed", 1);
        }
    }
    return sock_fd;
}

_Noreturn void start_server_socket(char *socket_file, int max_connects) {
    int sock_fd = make_named_socket(socket_file, false);

    /* listen for clients, up to MaxConnects */
    if (listen(sock_fd, max_connects) < 0) {
        log_msg("Listen call on the socket failed. Terminating.", true); /* terminate */
    }
    log_msg("Listening for client connections...\n", false);
    /* Listens indefinitely */
    while (1) {
        struct sockaddr_in caddr; /* client address */
        int len = sizeof(caddr);  /* address length could change */

        printf("Waiting for incoming connections...\n");
        int client_fd = accept(sock_fd, (struct sockaddr *) &caddr, &len);  /* accept blocks */

        if (client_fd < 0) {
            log_msg("accept() failed. Continuing to next.", 0); /* don't terminate, though there's a problem */
            continue;
        }
        /* Start a worker thread to handle the received connection. */
        if (!create_worker_thread(client_fd)) {
            log_msg("Failed to create worker thread. Continuing to next.", 0);
            continue;
        }

    }  /* while(1) */
}
void send_bmd_path_to_socket(char *msg, char *socket_file){
	/**
	 * TODO: This function can be implemented in a separate module
	 * which will also implement the socket server.
	 */
	//return 1; /* 1 => Success */
	int sockfd = make_named_socket(socket_file, true);

    /* Write some stuff and read the echoes. */
    log_msg("CLIENT: Connect to server, about to write some stuff...", false);
    if (write(sockfd, msg, strlen(msg)) > 0) {
        /* get confirmation echoed from server and print */
        char buffer[5000];
        memset(buffer, '\0', sizeof(buffer));
        if (read(sockfd, buffer, sizeof(buffer)) > 0) {
            printf("CLIENT: Received from server:: %s\n", buffer);
        }
    }
    log_msg("CLIENT: Processing done, about to exit...", false);
    close(sockfd); /* close the connection */
}

/**
 * Define a suitable struct for holding the endpoint request handling result.
 */
typedef struct
{
	int status;
	char *bmd_path;
} endpoint_result;

const char* my_sock_file = "./my_sock";

int esb_endpoint(struct http_request *);
endpoint_result save_bmd(struct http_request *);

/**
 * HTTP request handler function mapped in the conf file.
 */
int esb_endpoint(struct http_request *req)
{
	printf("Received the BMD request.\n");
	endpoint_result epr = save_bmd(req);
	if (epr.status < 0)
	{
		printf("Failed to handle the BMD request.\n");
		return (KORE_RESULT_ERROR);
	}
	else
	{
		int sock_status = send_bmd_path_to_socket(epr.bmd_path, my_sock_file);
		if (sock_status > 0)
		{
			kore_log(LOG_INFO, "BMD path sent to server socket.");
			return (KORE_RESULT_OK);
		}
		else
		{
			kore_log(LOG_ERR, "Failed to send BMD path sent to server socket.");
			return (KORE_RESULT_ERROR);
		}
	}
}

char *create_work_dir_for_request()
{
	kore_log(LOG_INFO, "Creating the temporary work folder.");
	/**
	 * Create a temporary folder in the current directory.
	 * Its name should be unique to each request.
	 */
	char *temp_path = malloc(PATH_MAX * sizeof(char));
	char *base_dir = "./bmd_files";
	sprintf(temp_path, "%s/%lu", base_dir, (unsigned long)time(NULL));
	if (mkdir(base_dir, 0700) != 0)
	{
		kore_log(LOG_ERR, "Failed to create folder %s.", base_dir);
	}
	if (mkdir(temp_path, 0700) != 0)
	{
		kore_log(LOG_ERR, "Failed to create folder %s.", temp_path);
	}
	else
	{
		kore_log(LOG_INFO, "Temporary work folder: %s", temp_path);
	}
	return temp_path;
}

void do_cleanup(endpoint_result *ep_res_ptr, char *bmd_file_path, int fd)
{
	if (close(fd) == -1)
		kore_log(LOG_WARNING, "close(%s): %s", bmd_file_path, errno_s);

	if (ep_res_ptr->status < 0)
	{
		kore_log(LOG_ERR, "We got an error. Deleteing the file %s", bmd_file_path);
		if (unlink(bmd_file_path) == -1)
		{
			kore_log(LOG_WARNING, "unlink(%s): %s",
					 bmd_file_path, errno_s);
		}
	}
	else
	{
		ep_res_ptr->status = 1;
		printf("BMD is saved\n");
	}
}

void write_data_to_file(struct http_request *req,
						endpoint_result *ep_res_ptr,
						char *bmd_file_path, int fd,
						struct http_file *file)
{
	u_int8_t buf[BUFSIZ];
	ssize_t ret, written;
	/* While we have data from http_file_read(), write it. */
	/* Alternatively you could look at file->offset and file->length. */
	for (;;)
	{
		ret = http_file_read(file, buf, sizeof(buf));
		if (ret == -1)
		{
			kore_log(LOG_ERR, "failed to read from file");
			http_response(req, 500, NULL, 0);
			ep_res_ptr->status = -1;
			goto cleanup;
		}

		if (ret == 0)
			break;

		written = write(fd, buf, ret);
		kore_log(LOG_INFO, "Written %d bytes to %s.", written, bmd_file_path);
		if (written == -1)
		{
			kore_log(LOG_ERR, "write(%s): %s",
					 bmd_file_path, errno_s);
			http_response(req, 500, NULL, 0);
			ep_res_ptr->status = -1;
			goto cleanup;
		}

		if (written != ret)
		{
			kore_log(LOG_ERR, "partial write on %s",
					 bmd_file_path);
			http_response(req, 500, NULL, 0);
			ep_res_ptr->status = -1;
			goto cleanup;
		}
	}

cleanup:
	do_cleanup(ep_res_ptr, bmd_file_path, fd);
}

endpoint_result
save_bmd(struct http_request *req)
{
	endpoint_result ep_res;
	/* Default to OK. 1 => OK, -ve => Errors */
	ep_res.status = 1;

	/* Only deal with POSTs. */
	if (req->method != HTTP_METHOD_POST)
	{
		kore_log(LOG_ERR, "Rejecting non POST request.");
		http_response(req, 405, NULL, 0);
		ep_res.status = -1;
		return ep_res;
	}

	/* Parse the multipart data that was present. */
	http_populate_multipart_form(req);

	/* Find our file. It is expected to be under parameter named bmd_file */
	struct http_file *file;
	if ((file = http_file_lookup(req, "bmd_file")) == NULL)
	{
		http_response(req, 400, NULL, 0);
		ep_res.status = -1;
		return ep_res;
	}

	/* Open dump file where we will write file contents. */
	char bmd_file_path[PATH_MAX];
	char *req_folder = create_work_dir_for_request();
	sprintf(bmd_file_path, "%s/%s", req_folder, file->filename);

	int fd = open(bmd_file_path, O_CREAT | O_TRUNC | O_WRONLY, 0700);
	if (fd == -1)
	{
		kore_log(LOG_ERR, "Failed to create the file at %s\n", bmd_file_path);
		http_response(req, 500, NULL, 0);
		ep_res.status = -1;
		return ep_res;
	}
	printf("BMD file to be saved at: %s\n", bmd_file_path);
	write_data_to_file(req, &ep_res, bmd_file_path, fd, file);
	if (ep_res.status > 0)
	{
		http_response(req, 200, NULL, 0);
		kore_log(LOG_INFO, "BMD file '%s' successfully received",
				 file->filename);
		ep_res.bmd_path = malloc(strlen(bmd_file_path) * sizeof(char) + 1);
		strcpy(ep_res.bmd_path, bmd_file_path);
	}
	return ep_res;
}

/*int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s [server|client] [Local socket file path] [Message to send (needed only in case of client)]\n",
               argv[0]);
        exit(-1);
    }
    if (0 == strcmp("server", argv[1])) {
        start_server_socket(argv[2], 10);
    } else {
        send_bmd_path_to_socket(argv[3], argv[2]);
    }
}*/
