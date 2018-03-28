#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
WP* new_WP();
void free_wp(int n);
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_WP(){
  if(free_ == NULL){
    return NULL;
  }
  WP* tempw = free_;
  if(head == NULL){
    head = tempw;
  }
  else{
    WP *t = head;
    while(t->next != NULL){
      t = t->next;
    }
    t->next = tempw;
  }
  tempw->next = NULL;
  return tempw;
}

void free_wp(int n){
  WP *temp = head;
  WP *wp = NULL;
  while(temp != NULL){
    if(temp->NO == n){
      wp = temp;
      break;
    }
    temp = temp->next;
  }
  if(wp == NULL){
    printf("wrong number\n");
    return ;
  }
  if(head == wp){
    head = wp->next;
  }
  else{
    WP *temp_w = head;
    while(temp_w->next != wp){
      temp_w = temp_w->next;
    }
    temp_w->next = wp->next;
    if(free_ == NULL){
      free_ = wp;
      wp->next = NULL;
    }
    else{
      WP *temp_f = free_;
      while(temp_f->next != NULL){
        temp_f = temp_f->next;
      }
      temp_f->next = wp;
      wp->next = NULL;
    }
  }
}

bool check_wp(){
  WP *temp = head;
  bool r = false;//the whole result
  while(temp != NULL){
    bool flag = false;
    uint32_t re = expr(temp->exp, &flag);
    if(flag == true){
      if(re == temp->value){
        temp = temp->next;
	continue;
      }
      else{
        r = true;
	printf("watchpoint %d : %s\n\n",temp->NO,temp->exp);
	printf("Old value = 0x%08x\nNew value = 0x%08x\n",temp->value, re);
	temp->value = re;
      }
    }
    else{
      printf("wrong expression");
    }
    temp = temp->next;
  }
  return r;
}

void show_used_wp(){
  if(head == NULL){
    printf("no watchpoint\n");
    return ;
  }
  WP *temp = head;
  printf("watchpoints exit, they are: \n");
  while(temp != NULL){
    printf("%-8d%s\n",temp->NO, temp->exp);
    temp = temp->next;
  }
}
