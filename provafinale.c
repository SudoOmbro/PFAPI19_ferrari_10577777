#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//type definitions--------------------------------------------------------------

typedef char String[20];

typedef struct {
  String name;
  void* first_popular_entity;
  void* table_next;
} Relation;

typedef struct {
  Relation* relation;
  void* dest_entity;
  void* table_next;
} Relation_in_entity;

typedef struct {
  String name;
  Relation_in_entity out_relations_table[500];
  void* table_next;
} Entity;

typedef struct {
  Entity* entity;
  void* list_next;
} Popular_entity;

Entity entity_table[80000];
Relation relation_table[30];

//------------------------------------------------------------------------------

int main(int argc, char const *argv[])
{
  if (argc == 2)
  {
    //program
  }
  return 0;
}
