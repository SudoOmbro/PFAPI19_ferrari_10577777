#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//type definitions--------------------------------------------------------------

typedef char LongString[160];
typedef char String[50];

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
  Relation_in_entity* out_relations_table[500];
  void* table_next;
} Entity;

typedef struct {
  Entity* entity;
  void* list_next;
} Popular_entity;

Entity* entity_table[80000];
Relation* relation_table[30];

//------------------------------------------------------------------------------
//functions definitions

int hash_function(int table_size, String text)
{
  int value;
  return value;
}
//data la grandezza della tabella e una stringa in input genera un valore

int get_argument(LongString input_string, char dest_string[50], int start_pos)
{
  char byte = input_string[start_pos];
  int string_length = 0;
  while (byte != ' ' && byte != 0) //while the character isn't a space or the STC
  {
    printf("length: %d\n", string_length);
    printf("char: %c\n", byte);
    dest_string[string_length] = byte;
    string_length ++;
    start_pos ++;
    byte = input_string[start_pos];
  }
  string_length --;
  dest_string[string_length] = '\0';
  return start_pos;
}
//data in ingresso la stringa in stdin, la analizza e riempie la stringa
//dest_string (argomento) con la stringa trovata a partire da start_pos
//fino ad uno spazio.

Entity* create_entity(String* name)
{
  Entity* self;
  self = (Entity*) malloc(sizeof(Entity));
  return self;
}

int generate_opcode(LongString input_string)
{
  int opcode = 5;
  if (input_string[0] == 'a') //addent or addrel
  {
    if (input_string[3] == 'e') //addent
      opcode = 0;
    else if (input_string[3] == 'r') //addrel
      opcode = 1;
  }
  else if (input_string[0] == 'd') //delent or delrel
  {
    if (input_string[3] == 'e') //delent
      opcode = 2;
    else if (input_string[3] == 'r') //delrel
      opcode = 3;
  }
  else if (input_string[0] == 'r') //report
    opcode = 4;

  return opcode;
}
//guarda i primi 6 caratteri della stringa in input per generare l'opcode
//case 0 : addent
//case 1 : addrel
//case 2 : delent
//case 3 : delrel
//case 4 : report
//case 5 : end

//------------------------------------------------------------------------------
int main() //main program
{
  String argument0;
  String argument1;
  String argument2;
  LongString input_string;
  int opcode;
  while (1)
  {
    fgets(input_string, 160, stdin);
    opcode = generate_opcode(input_string);

    switch (opcode)
    {
      case 0:
        #ifdef deb
        printf("case 0\n");
        #endif
        opcode = get_argument(input_string, argument0, 7);
        printf("argument: %s\n", argument0);
        printf("next argument position: %d\n", opcode);

        opcode = get_argument(input_string, argument1, opcode);
        printf("argument: %s\n", argument1);
        printf("next argument position: %d\n", opcode);

        break;

      case 1:
        #ifdef deb
        printf("case 1\n");
        #endif

        break;

      case 2:
        #ifdef deb
        printf("case 2\n");
        #endif

        break;

      case 3:
        #ifdef deb
        printf("case 3\n");
        #endif

        break;

      case 4:
        #ifdef deb
        printf("case 4\n");
        #endif

        break;

      case 5:
        #ifdef debug
        printf("case 5\n");
        #endif

        return 0;
        break;
    }
  }
}
