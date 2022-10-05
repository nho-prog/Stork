#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>
#include "esb.h"


int fetch_new_request_from_db(bmd *request)
{
    /** 
     * TODO: query the DB for this, and populate the 
     * request pointer with the requests.
     */

    printf("Checking for new requests in esb_requests table.\n");
    if (request == NULL){
        return -1;
    }

    return 1; // 1 => OK, -1 => Errors
}

/**
 * TODO: Implement the proper logic as per ESB specs.
 */
void *poll_database_for_new_requets(void *vargp)
{
    // Step 1: Open a DB connection
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    rc = sqlite3_open("test.db", &db);
    if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
    } else {
      fprintf(stdout, "Opened database successfully\n");
    }
    char sql[];

    sql="CREATE TABLE ESB_REQUEST("
   "ID INT PRIMARY KEY NOT NULL",
   "SENDER_ID VARCHAR(45) NOT NULL",
   "DEST_ID VARCHAR(45) NOT NULL",
   "MESSAGE_TYPE VARCHAR(45) NOT NULL",
   "REFERENCE_ID VARCHAR(45) NOT NULL",
   "MESSAGE_ID VARCHAR(45) NOT NULL",
   "RECIEVED_ON DATETIME NOT NULL",
   "DATA_LOCATION TEXT NOT NULL",
   "STATUS VARCHAR(20) NOT NULL",
   "STATUS_DETAILS TEXT NOT NULL);";

    sql="CREATE TABLE ROUTES("
   "ROUTE_ID INT PRIMARY KEY NOT NULL",
   "SENDER VARCHAR(45) TEXT NOT NULL",
   "DESTINATION VARCHAR(45) TEXT NOT NULL",
   "MESSAGE_TYPE VARCHAR(45) NOT NULL",
   "IS_ACTIVE BIT(1) NOT NULL);";

    sql="CREATE TABLE TRANSFORM_CONFIG("
   "ID INT PRIMARY KEY NOT NULL",
   "ROUTE_ID INT MOT NULL",
   "CONFIG_KEY VARCHAR(45) NOT NULL",
   "CONFIG_VALUE TEXT NOT NULL);";

    sql="CREATE TABLE TRANSPORT_CONFIG("
   "ID INT PRIMARY KEY NOT NULL",
   "ROUTE_ID INT NOT NULL",
   "CONFIG_KEY VARCHAR(45) NOT NULL",
   "CONFIG_VALUE TEXT NOT NULL);";

    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    } else {
      fprintf(stdout, "Tables created successfully\n");
    }
    sqlite3_close(db);

    int i = 0;
    while (i < 99)
    {
        i++;
        vargp[i];

        /**
         * Step 2: Query the esb_requests table to see if there
         * are any newly received BMD requets.
         */
        bmd req;
        /* Get the route_id to handle the request */
        int route_id = get_active_route_id(req->sender,
                                           req->destination,
                                           req->message_type);

        /* Get transformation and transportation configuration */
        transform_t *transform = fetch_transform_config(route_id);
        transport_t *transport = fetch_transport_config(route_id);

        /**
         * Step 3:
         */
        if (fetch_new_request_from_db(&req))
        {
            /**
              * Found a new request, so we will now process it.
              * See the ESB specs to find out what needs to be done
              * in this step. Basically, you will be doing:
              * 1. Find if there is any transformation to be applied to
              *    the payload before transporting it to destination.
              * 2. If needed, transform the request.
              * 3. Transport the transformed data to destination.
              * 4. Update the status of the request in esb_requests table
              *    to mark it as done (or error) depending on the outcomes
              *    of this step.
              * 5. Cleanup
              */
            printf("Applying transformation and transporting steps.\n");
        }
        /**
         * Sleep for polling interval duration, say, 5 second.
         * DO NOT hard code it here!
         */
        printf("Sleeping for 5 seconds.\n");
        sleep(5);
    }
}
