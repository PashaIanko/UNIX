#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/resource.h>
 
#define MAX_BUF 200
extern char* optarg;
extern char** environ;


 int main(int argc, char *argv[])
 {
     char options_delimeter[] = "------------";
     char options[] = "f:dg:ispuU:cC:dvV:";  /* valid options */
     int c, invalid = 0, dflg = 0, fflg = 0, gflg = 0;

     int setpgid_res = 0, get_rlimit_res = 0;

     //for u option
     struct rlimit rlim;

     //option U
     char* str_end;
     rlim_t new_rlim_cur;
     struct rlimit new_rlim_; 
     unsigned long soft_limit_ = 0, hard_limit_ = 0;

     //option c
     struct rlimit core_limit;
     int success = 0;

     //option C
     struct rlimit new_rlim;
     char* str_end_;
     unsigned long soft_limit = 0, hard_limit = 0;
     int C_success = 0;

     //option d
     char dir_arr[MAX_BUF];

     //option V
     char* new_tz_string = NULL; /*"TZ=PST8UTD"*/
     int code = 0;

     //option v
     char** p = NULL;
     char *f_ptr, *g_ptr;
 
     printf("argc equals %d\n", argc);
     while ((c = getopt(argc, argv, options)) != EOF) {
      switch (c) {
      /*case 'd':
          dflg++;
          break;*/
      case 'f':
          fflg++;
          f_ptr = optarg;
          break;
      case 'g':
          gflg++;
          g_ptr = optarg;
          break;
      case 'i':
	  printf("%s\n", options_delimeter);
	  printf("uid = %d\nueid = %d\ngid = %d\ngeid = %d\n", getuid(), geteuid(), getgid(), getegid());
	  printf("%s\n", options_delimeter);
	  
          break;
      case 's':
	  /*process group leader - process with proc_indicator == group_indicator*/
	  printf("%s\nexecuting -s option\n", options_delimeter);
	  setpgid_res = setpgid(getpid(), getpid()); //now pgid == pid
	  if(setpgid_res)
	       return setpgid_res;
	  printf("%s\n", options_delimeter);
	  break;
      case 'p':
	  printf("%s\n", options_delimeter);
	  printf("pid = %d\nppid = %d\npgid = %d\n", getpid(), getppid(), getpgrp());
	  printf("%s\n", options_delimeter);
	  break;
      case 'u':
	  printf("%s\n", options_delimeter);	  
	  get_rlimit_res = getrlimit(RLIMIT_CORE, &rlim);
	  if(get_rlimit_res){
		perror("option u: getrlimit: unsuccessful\n");
		break;
	  }
	  printf("soft limit = %lu\nhard limit = %lu\n", rlim.rlim_cur, rlim.rlim_max);
	  printf("%s\n", options_delimeter);
	  break;
      case 'U':
	  printf("%s\nSetting -U option - CORE LIMIT\n", options_delimeter);	  	  
	  new_rlim_cur = strtoul(optarg, &str_end, 10);
	  new_rlim_.rlim_cur = new_rlim_cur;
	  if(setrlimit(RLIMIT_CORE, &new_rlim_)) {
		perror("Option U: failed to set new rlimit\n");
	  }
	  printf("%s\n", options_delimeter);
	  break;
      case 'c':
	  printf("%s\n", options_delimeter);
	  success = getrlimit(RLIMIT_CORE, &core_limit);
	  if(success) {
		perror("option c: failed to get core resourse limit\n");
	  	printf("%s\n", options_delimeter);
		return success;
	  }
	  printf("soft_lim:%ld\nhard_lim:%ld\n", core_limit.rlim_cur, core_limit.rlim_max);
	  printf("%s\n", options_delimeter);
	  break;
      case 'C':
	  printf("%s\nSetting the core sorft and hard limits - -C option\n", options_delimeter);
	  new_rlim.rlim_cur = strtoul(optarg, &str_end_, 10);
	  new_rlim.rlim_max = strtoul(optarg, &str_end_, 10);
	  
	  C_success = setrlimit(RLIMIT_CORE, &new_rlim);
	  if(C_success) {
		perror("Option -C: failed to set new rlimit\n");
		printf("%s\n", options_delimeter);
		return C_success;
	  }
	  else
	  	printf("%s\n", options_delimeter);
		break;
      case 'd':
	  printf("%s\n", options_delimeter);
	  if(getcwd(dir_arr, MAX_BUF) == NULL){
		perror("unable to print a directory\n");
		printf("%s\n", options_delimeter);
		
	  }
	  else {
	  	printf("Dir: %s\n", dir_arr);
		printf("%s\n", options_delimeter);		
	  }
	  break;
      case 'v':
	  printf("%s\n", options_delimeter);
	  for(p = environ;*p!=NULL; ++p) {
	  	printf("%s\n", *p);
	  }
	  printf("%s\n", options_delimeter);
	  break;
      case 'V':
	  printf("%s\n", options_delimeter);
	  printf("Setting new environment variable, option -V\n");
	  new_tz_string = optarg;
	  code = putenv(new_tz_string);
	  if(code) {
		perror("option V: couldn't set the environmental varialbe\n");
		printf("%s\n", options_delimeter);
	  }
	  printf("%s\n", options_delimeter);
	  break;
	  

      case '?':
	  printf("%s\n", options_delimeter);
          printf("invalid option is %c\n", optopt);
          invalid++;
	  printf("%s\n", options_delimeter);
      }
     }
     printf("dflg equals %d\n", dflg);
     if(fflg)
      printf("f_ptr points to %s\n", f_ptr);
     if(gflg)
      printf("g_ptr points to %s\n", g_ptr);
     printf("invalid equals %d\n", invalid);
     printf("optind equals %d\n", optind);
     if(optind < argc)
      printf("next parameter = %s\n", argv[optind]);
     return 0;

 }
