#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUB_REL_TABLE_SIZE 499
#define ENTITY_TABLE_SIZE 91711
#define RELATIONS_TABLE_SIZE 43

/* add -Ddeb to gcc compiler options to launch in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char LongString[160];
typedef char String[50];

typedef struct {
  String name;
  void* sub_relations[SUB_REL_TABLE_SIZE];  //SubRelation*
  void* most_popular_entity;  //MSE*
  void* table_next; //Relation*
} Relation;

typedef struct {
  String name;
  void* table_next; //Entity*
} Entity;

typedef struct {
  Entity* source;
  Entity* destination;
  void* table_next; //SubRelation*
} SubRelation;

typedef struct {
  Entity* entity;
  int times;
  void* next; //MSE*
} MSE;

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
  printf("hash value: %d\n", value);
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

Relation* create_relation(String name)
{
  Relation* self;
  self = (Relation*) malloc(sizeof(Relation));
  strcpy(self->name, name);
  return self;
}
//crea relazione, assegna il nome e ritorna il puntatore.

SubRelation* create_sub_relation(Entity* e1, Entity* e2)
{
  SubRelation* self = (SubRelation*) malloc(sizeof(SubRelation));
  self->source = e1;
  self->destination = e2;
  self->table_next = NULL;
  return self;
}

int relation_add_entities(Relation* rel, Entity* e1, Entity* e2)
{
  int pos = strcmp(e1->name, e2->name) % SUB_REL_TABLE_SIZE;
  if (rel->sub_relations[pos] == NULL)
  {
    rel->sub_relations[pos] = create_sub_relation(e1, e2);
    #ifdef deb
      printf("Added entries to relation\n");
    #endif
  }
  else
  {
    SubRelation* sub_rel = (SubRelation*) rel->sub_relations[pos];
    while(sub_rel != NULL)
    {
      if (e1 == sub_rel->source && e2 == sub_rel->destination)
      {
        #ifdef deb
          printf("Relation already exists\n");
        #endif
        return 1;
      }
      sub_rel = (SubRelation*) sub_rel->table_next;
    }
    sub_rel = create_sub_relation(e1, e2);
    #ifdef deb
      printf("Added entries to relation\n");
    #endif
  }
  return 0;
}
//ritorna 0 se tutto ok, 1 altrimenti

Entity* handle_entity_creation(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  if (entity_table[pos] == NULL)
  {
    entity_table[pos] = create_entity(name);
    #ifdef deb
      printf("Created new entity in empty slot\n");
    #endif
    return entity_table[pos];
  }
  else
  {
    Entity* entity = entity_table[pos];
    while (entity != NULL)
    {
      if (strcmp(name, entity->name) == 0)
      {
        #ifdef deb
          printf("Entity already exists\n");
        #endif
        return 0;
      }
      entity = (Entity*) entity->table_next;
    }
    #ifdef deb
      printf("Created new entity\n");
    #endif
    entity = create_entity(name);
    return (Entity*) entity->table_next;
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle entity creation

Relation* handle_relation_creation(String name1, String name2, String name)
{
  int pos = hash_function(name, RELATIONS_TABLE_SIZE);
  Entity* e1 = get_entity(name1);
  Entity* e2 = get_entity(name2);

  if (e1 == NULL || e2 == NULL) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  if (relations_table[pos] == NULL)
  {
    relations_table[pos] = create_relation(name);
    #ifdef deb
      printf("Created new relation in empty slot\n");
    #endif
    relation_add_entities(relations_table[pos], e1, e2);
    return relations_table[pos];
  }
  else
  {
    Relation* rel = relations_table[pos];
    while (rel != NULL)
    {
      if (strcmp(name, rel->name) == 0)
      {
        relation_add_entities(rel, e1, e2);
        return rel;
      }
      rel = (Relation*) rel->table_next;
    }
    rel = create_relation(name);
    #ifdef deb
      printf("Created new relation\n");
    #endif
    relation_add_entities(rel, e1, e2);
    return (Relation*) rel->table_next;
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

void report_function()
{

}

//debug functions---------------------------------------------------------------

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

//------------------------------------------------------------------------------

int generate_opcode(LongString input_string)
{
  if (input_string[0] == 'a') //addent or addrel
  {
    if (input_string[3] == 'e') //addent
      return 0;
    else //addrel
      return 1;
  }
  else if (input_string[0] == 'd') //delent or delrel
  {
    if (input_string[3] == 'e') //delent
      return 2;
    else //delrel
      return 3;
  }
  else if (input_string[0] == 'r') //report
    return 4;

  return 5; //end
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
      case 0: //addent
      {
        get_argument(input_string, argument0, 7);
        handle_entity_creation(argument0);
        #ifdef deb
          deb_print_entities();
        #endif
        break;
      }
      case 1: //addrel
      {
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);
        handle_relation_creation(argument0, argument1, argument2);
        break;
      }
      case 2: //delent
      {
        opcode = get_argument(input_string, argument0, 7);

        break;
      }
      case 3: //delrel
      {
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);

        break;
      }
      case 4: //report
      {
        report_function();
        break;
      }
      case 5: //end
      {
        return 0;
        break;
      }
    }
  }
}
