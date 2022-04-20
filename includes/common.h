#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "findUrls.h"
#include <sys/wait.h>
#include <signal.h>
#include <fstream>

#include "workerFunc.h"
#include <string.h>