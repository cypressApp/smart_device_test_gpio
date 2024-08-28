#include "context.h"

bool is_checksum_correct(char *data , int start_index , int end_index , int checksum_index){

    long int temp_sum = 0;

    for(int i = start_index ; i <= end_index ; i++){
        temp_sum += data[i];
    }

    if(data[checksum_index] != (temp_sum & 0xFF)){
        return false;
    }

    return true;
    

}