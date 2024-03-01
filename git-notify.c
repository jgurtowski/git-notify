#include <stdio.h>
#include <string.h>

#include <git2.h>
#include <mailutils/mailutils.h>

size_t git_repo_num_changes(char *repo_path){

  git_repository *repo = NULL;
  if(git_repository_open(&repo, repo_path) != 0){
    printf("Could not open Git Repo: %s\n", repo_path);
    exit(1);
  }
  
  git_status_options status_ops = GIT_STATUS_OPTIONS_INIT;
  status_ops.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;
  
  git_status_list *status_list = NULL;
  git_status_list_new(&status_list,
		      repo,
		      &status_ops);
  int num_diffs = git_status_list_entrycount(status_list);

  git_status_list_free(status_list);
  git_repository_free(repo);

  return num_diffs;
}

int send_notification(const char **to_addresses, const char *message_text){
  mu_message_t msg = 0;
  mu_mailer_t mailer = 0;
  mu_address_t from = 0;
  mu_address_t to = 0;
  mu_stream_t in = 0;

  char *mailertype = "sendmail:";

  mu_registrar_record (mu_sendmail_record);

  MU_ASSERT (mu_static_memory_stream_create(&in, message_text,
					    strlen(message_text)));
  MU_ASSERT (mu_message_create(&msg, NULL));
  MU_ASSERT (mu_message_set_stream (msg, in, NULL));

  MU_ASSERT (mu_address_createv (&to, to_addresses, 1));
  
  MU_ASSERT (mu_mailer_create(&mailer, mailertype));

  //mu_debug_set_category_level (MU_DEBCAT_MAILER, 
  //MU_DEBUG_LEVEL_UPTO (MU_DEBUG_PROT));

  MU_ASSERT (mu_mailer_open (mailer, 0));
  MU_ASSERT (mu_mailer_send_message (mailer, msg, from, to));
}

int main(int argc, char *argv[]){

  if(!(argc > 1)){
    printf("Usage: git-notify /path/to/repo [/path/to/repo ..]\n");
    return 1;
  }

  const char *to_addresses[] = {"james"};
  const char *base_message_text = {"Subject: Uncommitted Changes \n\nThere are uncommitted changes in git repo "};
  size_t num_changes;
  
  git_libgit2_init();
  
  for(int i = 1; i < argc; ++i){
    char *repo_path = argv[i];
    num_changes = git_repo_num_changes(repo_path);
    if(num_changes != 0){
      char *message_text = malloc(strlen(base_message_text) + strlen(repo_path) + 1);
      strcpy(message_text, base_message_text);
      strcat(message_text, repo_path);
      send_notification(to_addresses, message_text);
      free(message_text);
    }
  }

  git_libgit2_shutdown();
    
  return 0;
}
