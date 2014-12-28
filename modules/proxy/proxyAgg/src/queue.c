/* 
 * 
 * Filename: queue.c
 * Author: Nipun Arora
 * Created: Fri Dec 26 23:24:32 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: C Program to Implement various Queue Functions using Dynamic Memory Allocation
 */

#include <stdio.h>
#include <stdlib.h>
#define MAX 20

typedef enum {TRUE = 1, FALSE = 0} bool;
 
struct node
{
  char *data;
  struct node *link;
}*front, *rear;
 
// function protypes
void insert();
void delete();
void queue_size();
bool pop_and_delete();

/*
void main(){
  int choice, value;
 
  while(1)
    {
      printf("enter the choice \n");
      printf("1 : create an empty queue \n2 : Insert element\n");
      printf("3 : Dequeue an element \n4 : Check if empty\n");
      printf("5. Get the first element of the queue\n");
      printf("6. Get the number of entries in the queue\n");
      printf("7. Exit\n");
      scanf("%d", &choice);
      switch (choice)    // menu driven program
        {
        case 1: 
          printf("Empty queue is created with a capacity of %d\n", MAX);
          break;
        case 2:    
          insert();
          break;
        case 3: 
          delete();
          break;
        case 4: 
          check();
          break;
        case 5: 
          first_element();
          break;
        case 6: 
          queue_size();
          break;
        case 7: 
          exit(0);
        default: 
          printf("wrong choice\n");
          break;
        }
    }
}
*/

// to insert elements in queue
void insert(char *data){
  struct node *temp;

  &temp->data;
  temp->link = NULL;
  if (rear  ==  NULL)
    {
      front = rear = temp;
    }
  else
    {
      rear->link = temp;
      rear = temp;
    }    
}

/* pop and delete data */
bool pop_and_delete(){
  struct node *temp;
  char *data;
  temp = front;
  bool flag = FALSE;

  if (front == NULL)
    {
      printf("queue is empty \n");
      front = rear = NULL;
      flag = FALSE;
    }
  else
    {    
      data = front->data;
      front = front->link;
      free(temp);
      flag = TRUE;
    }
  return flag;
}
 
// delete elements from queue
void delete(){

  struct node *temp;
  char *data;
  temp = front;
  if (front == NULL){
    printf("queue is empty \n");
    front = rear = NULL;
  }
  else{    
    data = front->data;
    front = front->link;
    free(temp);
  }
}
  
// returns number of entries and displays the elements in queue
void queue_size(){
  struct node *temp;
 
  temp = front;
  int cnt = 0;
  if (front  ==  NULL){
    printf(" queue empty \n");
  }
  while (temp){
    temp = temp->link;
    cnt++;
  }
  printf("********* size of queue is %d ******** \n", cnt);
}
