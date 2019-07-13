#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT_REL_TABLE_SIZE 499
#define ENTITY_TABLE_SIZE 91711
#define RELATIONS_TABLE_SIZE 43

//type definitions--------------------------------------------------------------

typedef char LongString[160];
typedef char String[50];

typedef struct {
  String name;
  void* first_popular_entity;
  void* table_next;
} Relation;

typedef struct {
  Relation* name;
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

Entity* entity_table[ENTITY_TABLE_SIZE];          // = table 0
Relation* relations_table[RELATIONS_TABLE_SIZE];  // = table 1

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

int hash_function(char* text, const int table_size)
{
  int value = 0;
  char byte = text[0];
  for (int i=0; byte != '\0'; i++)
  {
    value += byte*(i+1)+byte;
    byte = text[i];
  }
  value =  value % table_size;
  #ifdef deb
  printf("%d\n", value);
  #endif
  return value;
}
//data la grandezza della tabella e una stringa in input genera un valore
//nel range della grandezza della tablella.

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

void* global_hash_table_linear_search(String name, int table_pos, int table)
{
  switch(table)
  {
    case 0: //entity table
    {
      Entity* entity = entity_table[table_pos];
      while (entity != NULL)
      {
        if (strcmp(name, entity->name) == 0)
          return entity;
        entity = (Entity*) entity->table_next;
      }
      break;
    }
    case 1: //relations table
    {
      Relation* rel = relations_table[table_pos];
      while (rel != NULL)
      {
        if (strcmp(name, rel->name) == 0)
          return rel;
        rel = (Relation*) rel->table_next;
      }
      break;
    }
  }
  return 0;
}
//returns 0 if nothing is found, else it returns the pointer
//of the wanted thing

Entity* get_entity(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  Entity* entity = (Entity*) global_hash_table_linear_search(name, pos, 0);
  if (entity != 0)
    return entity;
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

Relation* get_relation(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  Relation* entity = (Relation*) global_hash_table_linear_search(name, pos, 1);
  if (entity != 0)
    return entity;
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return relation.

Entity* create_entity(String name)
{
  Entity* self;
  self = (Entity*) malloc(sizeof(Entity));
  strcpy(self->name, name);
  return self;
}
//crea entità, assegna il nome e ritorna il puntatore.

Entity* handle_entity_creation(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  #ifdef deb
  printf("line: %d\n", pos);
  #endif
  if (entity_table[pos] == NULL)
  {
    entity_table[pos] = create_entity(name);
    return entity_table[pos];
  }
  else
  {
    Entity* entity = entity_table[pos];
    while (entity != NULL)
    {
      if (strcmp(name, entity->name) == 0)
        return 0;
      entity = (Entity*) entity->table_next;
    }
    entity->table_next = create_entity(name);
    return (Entity*) entity->table_next;
  }
  return 0;
}
//handle entity creation

//debug functions-------------------------------------------------------------
#ifdef deb
void deb_print_entities()
{
  for (int i=0; i<ENTITY_TABLE_SIZE; i++)
  {
    if (entity_table[i] != NULL)
    {
      Entity* entity = entity_table[i];
      printf("%d: ", i);
      while (entity != NULL)
      {
        printf("%s, ", entity->name);
        entity = entity->table_next;
      }
      printf("\n");
    }
  }
}
#endif

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
//guarda i primi 6 caratteri della stringa in input per generare l'opcode.

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
    {
        get_argument(input_string, argument0, 7);
        Entity* entity = handle_entity_creation(argument0);
        #ifdef deb
        if (entity != 0)
          printf("Entity created\n");
        else
          printf("Entity already exists\n");
        deb_print_entities();
        #endif
        break;
      }
      case 1:
      {
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
      }
      case 3:
      {
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);

        break;
      }
      case 4:
      {

        break;
      }
      case 5:
      {
        return 0;
        break;
      }
    }
  }
}
