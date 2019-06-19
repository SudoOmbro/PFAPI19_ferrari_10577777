#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//command definitions-----------------------------------------------------------

static const char command_report[] = "report"; //case 0
static const char command_addent[] = "addent"; //case 1
static const char command_delent[] = "delent"; //case 2
static const char command_addrel[] = "addrel"; //case 3
static const char command_delrel[] = "delrel"; //case 4
static const char command_end[] = "end";       //case 5

//type definitions--------------------------------------------------------------

typedef char String[50];
typedef char LongString[200];

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
//functions definitions

int hash_function(int table_size, String text)
{
  int value;
  return value;
}

Entity* create_entity()
{
  Entity* self;
  return self;
}

char generate_opcode(LongString input_string)
{
  //guarda i primi 6 caratteri della stringa in input per generare l'opcode
  char opcode;
  return opcode;
}

//------------------------------------------------------------------------------
int main() //main program
{
  LongString input_string;
  char opcode = 0;
  while (true)
  {
    scanf("%s", input_string);
    opcode = generate_opcode(input_string);
    switch (opcode)
    {
      case 0:
      {

      }
      case 1:
      {

      }
      case 2:
      {

      }
      case 3:
      {

      }
      case 4:
      {

      }
      case 5:
      {
        break;
      }
    }
  }

  return 0;
}
