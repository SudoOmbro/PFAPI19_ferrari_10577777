#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT_REL_TABLE_SIZE 500
#define ENTITY_TABLE_SIZE 80000
#define RELATIONS_TABLE_SIZE 30

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
  Relation_in_entity* out_relations_table[OUT_REL_TABLE_SIZE];
  void* table_next;
} Entity;

typedef struct {
  Entity* entity;
  void* list_next;
} Popular_entity;

Entity* entity_table[ENTITY_TABLE_SIZE];
Relation* relations_table[RELATIONS_TABLE_SIZE];

//------------------------------------------------------------------------------
//functions definitions

int hash_function(char* text, const int table_size)
{
  int i;
  int value = 0;
  char byte = text[0];
  for (i=0; byte != '\0'; i++)
  {
    value += byte*(i+i+1)+byte;
    byte = text[i];
  }
  value = (value << 1)% table_size;
  printf("%d\n", value);
  while (value > table_size)
    value -= table_size;
  return value;
}
//data la grandezza della tabella e una stringa in input genera un valore
//nel range della grandezza della tablella.
//entity: 91711
//relation: 43
//out_rel: 877

int get_argument(LongString input_string, char* dest_string, int start_pos)
{
  char byte = input_string[start_pos];
  int string_length = 0;
  while (byte != ' ' && byte != '\n' && byte != '\0') //while the char is valid
  {
    dest_string[string_length] = byte;
    string_length ++;
    start_pos ++;
    byte = input_string[start_pos];
  }
  dest_string[string_length] = '\0';
  return start_pos+1;
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
    else //addrel
      opcode = 1;
  }
  else if (input_string[0] == 'd') //delent or delrel
  {
    if (input_string[3] == 'e') //delent
      opcode = 2;
    else //delrel
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

void malloc_cleanup()
{

}

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
        get_argument(input_string, argument0, 7);
        opcode = hash_function(argument0, ENTITY_TABLE_SIZE);
        #ifdef deb
        printf("string: %s, size: %d, position: %d\n", argument0, ENTITY_TABLE_SIZE, opcode);
        #endif
        break;

      case 1:
        get_argument(input_string, argument0, 7);
        opcode = hash_function(argument0, RELATIONS_TABLE_SIZE);
        #ifdef deb
        printf("string: %s, size: %d, position: %d\n", argument0, RELATIONS_TABLE_SIZE, opcode);
        #endif

        break;

      case 2:
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);

        break;

      case 3:
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);

        break;

      case 4:

        break;

      case 5:

        return 0;
        break;
    }
  }
}
