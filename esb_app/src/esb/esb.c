#include<stdio.h>
#include "email.h"
#include "esb.h"
#include<stdbool.h>
/**
 * TODO: This is to be implemented separately.
 */
bmd que[101];
int front,rear=-1;
bmd parse_bmd_xml(char* bmd_file_path) {
    bmd b;
    bmd_envelop envl;
    envl.sender_id = "TEST-GUID-1";
    envl.destination_id = "TEST-GUID-2";
    envl.message_type = "TEST-GUID-3";

    b.envelop = envl;
    b.payload = "Some data here";
    return b;
}

int is_bmd_valid(bmd b)
{
    int valid = 1; // 1 => vaild, -1 => invalid
    // TODO: Implement the validation logic here
    if(!b.envelop.destination_id || !b.envelop.message_type || !b.envelop.sender_id ) 
    {
        printf("BMD must have [sender_id,destination_id and msg_type]");
        valid=-1;
    }

    return valid;
}
bool isQfull()
{
    if(rear == 101)
     return true;
    else return false;

}
void enQueue(bmd b)
{
    if(front == -1)
     front = 0;
    rear++;
    que[rear] = b;
}
int queue_the_request(bmd b)
{
    int success = 1; // 1 => OK, -1 => Error cases
    /** 
     * TODO: Insert the envelop data into esb_requests table,
     * and implement other logic for enqueueing the request
     * as specified in Theory of Operation.
     */
    if(is_bmd_valid(b))
     {
        if(isQfull())
        {
            success = -1;
        }
        else{
            enQueue(b);
        }

     }
     else{
        success = -1;
     }
    return success;
}

/**
 * This is the main entry point into the ESB. 
 * It will start processing of a BMD received at the HTTP endpoint.
 */
int process_esb_request(char* bmd_file_path) {
    int status = 1; // 1 => OK, -ve => Errors
    printf("Handling the BMD %s\n", bmd_file_path);
    /** TODO: 
     * Perform the steps outlined in the Theory of Operation section of
     * the ESB specs document. Each major step should be implemented in
     * a separate module. Suitable unit tests should be created for all
     * the modules, including this one.
     */
    // Step 1:
    bmd b = parse_bmd_xml(bmd_file_path);

    // Step 2:
    if (!is_bmd_valid(b))
    {
        //TODO: Process the error case
        printf("BMD is invalid!\n");
        status = -2;
    }
    else
    {
        // Step 3:
        status = queue_the_request(b);
    }
    
    return status;
}
