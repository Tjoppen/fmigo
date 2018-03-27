#include "common/fmigo_storage.h"
#include "common/common.h"

jm_log_level_enu_t fmigo_loglevel = jm_log_level_warning;

int main(){

    fmigo_storage::FmigoStorage fmudata;
    fmudata.test_functions();

  return 0;
}
