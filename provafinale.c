#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITY_TABLE_SIZE  31793
#define RELATIONS_TABLE_SIZE 7589
#define SUB_RELATIONS_ARRAY_SIZE 16384
#define RELATIONS_BUFFER_SIZE 4096
#define COLLISION_BUFFER_SIZE 3300

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char SuperLongString[4096];
typedef char LongString[160];
typedef char String[50];

typedef struct {
  String name;
  int hash_value;
  int position;
  int collision_pos;
  int sub_relation_number;
  void* sub_rel_array[SUB_RELATIONS_ARRAY_SIZE];
} Relation;

typedef struct {
  String name;
  int entity_id; //add 1 when entity changes.
  int valid;     // = 1 when valid, else = 0.
} Entity;

typedef struct {
  Relation* relation;
  Entity* source;
  Entity* destination;
  int source_id;
  int destination_id;
  void* table_next; //SubRelation*
} SubRelation;  //the destination is the entity that contains it.

typedef struct {
  Entity* entity;
  int occurrences;
} Pop; //struct used during report

int number_of_entities = 0;
Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE];          // = table 0
Relation* relations_table[RELATIONS_TABLE_SIZE][COLLISION_BUFFER_SIZE];  // = table 1

Relation* relations_buffer[RELATIONS_BUFFER_SIZE];
int number_of_relations = 0;

//declarations of data structures used in report
Pop entities_array[RELATIONS_BUFFER_SIZE][SUB_RELATIONS_ARRAY_SIZE];
Relation* relations_array[RELATIONS_BUFFER_SIZE];
int maximums[RELATIONS_BUFFER_SIZE];

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

int entity_hash_function(char* text)
{
  int value = 1;
  for (int i=0; text[i] != '\0'; i++)
  {
    value *= (text[i]*(i+1));
  }
  #ifdef deb
    printf("entity hash value: %d\n", abs((value) % ENTITY_TABLE_SIZE));
  #endif
  return abs((value) % ENTITY_TABLE_SIZE);;
}

int relation_hash_function(char* text)
{
  int value = 0;
  for (int i=0; text[i] != '\0'; i++)
  {
    value += (text[i]*(i+1));
  }
  #ifdef deb
    printf("relation hash value: %d\n", abs((value) % RELATIONS_TABLE_SIZE));
  #endif
  return abs((value) % RELATIONS_TABLE_SIZE);;
}

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

Entity* entity_hash_table_linear_search(String name, int table_pos)
{
  for(int i=0; i<COLLISION_BUFFER_SIZE; i++)
  {
    if (entity_table[table_pos][i] != NULL)
    {
      if (strcmp(name, entity_table[table_pos][i]->name) == 0)
      {
        #ifdef deb
          printf("linear search done, found entity\n");
        #endif
        return entity_table[table_pos][i];
      }
    }
    else
      break;
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
  return 0;
}

Relation* relation_hash_table_linear_search(String name, int table_pos)
{
  for(int i=0; i<COLLISION_BUFFER_SIZE; i++)
  {
    if (relations_table[table_pos][i] != NULL)
    {
      if (strcmp(name, relations_table[table_pos][i]->name) == 0)
      {
        #ifdef deb
          printf("linear search done, found relation\n");
        #endif
        return relations_table[table_pos][i];
      }
    }
    else
      break;
  }
  return 0;
}

int check_relation_validity(SubRelation* sub_rel)
{
  Entity* src = sub_rel->source;
  Entity* dest = sub_rel->destination;
  if (src->valid == 1 && dest->valid == 1)
    if (src->entity_id == sub_rel->source_id && dest->entity_id == sub_rel->destination_id)
      return 1;
  return 0;
}
//check the validity of a relation (1 = valid, 0 = not valid).

Entity* get_entity(String name)
{
  int pos = entity_hash_function(name);
  return entity_hash_table_linear_search(name, pos);
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

Relation* get_relation(String name)
{
  int pos = relation_hash_function(name);
  return relation_hash_table_linear_search(name, pos);
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return relation.

Entity* create_entity(String name, int table_pos)
{
  //create the entity in the hash table
  Entity* self;
  self = (Entity*) malloc(sizeof(Entity));
  strcpy(self->name, name);
  self->entity_id = 0;
  self->valid = 1;

  number_of_entities ++;
  return self;
}
//crea entità, assegna il nome e ritorna il puntatore.

void reallocate_entity(Entity* self, String name)
{
  strcpy(self->name, name);
  self->valid = 1;
  number_of_entities ++;
}

void deallocate_entity(Entity* self)
{
  self->entity_id ++;
  self->valid = 0;
  strcpy(self->name, "");
  number_of_entities --;
}

void sub_rel_array_fixup(SubRelation** array, int start_pos, const int max, SubRelation* replacer)
{
  SubRelation* temp;
  for (int i=start_pos; i<max; i++)
  {
    temp = array[i];
    array[i] = replacer;
    replacer = temp;
  }
}
//given the pointer to the array to restore, the starting position, the
//end postion and the entity that replaces, this function restores
//the order in the array.

void rel_buffer_fixup(int start_pos, Relation* replacer)
{
  Relation* temp;
  for (int i=start_pos; i<number_of_relations; i++)
  {
    replacer->position = i;
    temp = relations_buffer[i];
    relations_buffer[i] = replacer;
    replacer = temp;
  }
}

void sub_rel_array_fixup_delete(SubRelation** array, int start_pos, int end)
{
  int i;
  for (i=start_pos; i<end; i++)
  {
    array[i] = array[i+1];
  }
}
//given the pointer to the array to restore, the starting position
//and the end postion, this function restores the order in the array.

void sub_rel_array_fixup_delete_optimized(SubRelation** array, int end)
{
  #ifdef deb
    printf("array fixup optimized\n");
  #endif

  int i, nulls = 0;

  for(i=0; i<end; i++)
  {
    if (array[i] == NULL)
    {
      nulls = 1;
      break;
    }
  }

  while (i < end)
  {
    if (array[i+nulls] != NULL)
    {
      array[i] = array[i+nulls];
      i ++;
    }
    else
    {
      nulls ++;
    }
  }
}
//returns the number of deleted sub_releations.

void rel_buffer_fixup_delete(int start_pos)
{
  int i;
  for (i=start_pos; i<number_of_relations; i++)
  {
    relations_buffer[i] = relations_buffer[i+1];
    relations_buffer[i]->position = i;
  }
  relations_buffer[i] = NULL;
}

Relation* create_relation(String name, int hash, int pos)
{
  //create the relation to put in hash table
  int i;
  #ifdef deb
    printf("executing malloc...\n");
  #endif
  Relation* self = (Relation*) malloc(sizeof(Relation));
  #ifdef deb
    printf("malloc executed.\n");
  #endif
  strcpy(self->name, name);
  self->hash_value = hash;
  self->collision_pos = pos;
  self->sub_relation_number = 0;
  //create ordered list entry
  if (number_of_relations == 0)
  {
    relations_buffer[0] = self;
    number_of_relations ++;
    return self;
  }
  for (i=0; i<number_of_relations; i++)
  {
    if (strcmp(self->name, relations_buffer[i]->name) < 0)
    {
        #ifdef deb
          printf("relation is smaller\n");
        #endif
        number_of_relations ++;
        rel_buffer_fixup(i, self);
        return self;
    }
  }
  #ifdef deb
    printf("relation is bigger\n");
  #endif
  relations_buffer[i] = self;
  self->position = i;
  number_of_relations ++;
  return self;
}
//Create relation in hash table, create relation in ordered list and
//return the pointer to the relation in hash table.

SubRelation* create_sub_relation(Entity* src, Relation* rel, Entity* dest)
{
  SubRelation* self = (SubRelation*) malloc(sizeof(SubRelation));
  self->source = src;
  self->relation = rel;
  self->destination = dest;
  self->source_id = src->entity_id;
  self->destination_id = dest->entity_id;
  return self;
}
//creates a sub relation and returns the pointer to that.

int relation_add_entities(Relation* rel, Entity* src, Entity* dest, int pos)
{
  //setup
  int i;
  int rel_num = rel->sub_relation_number;
  SubRelation* sub_rel;
  SubRelation* pointer;
  SubRelation** array =  (SubRelation**) rel->sub_rel_array;
  if (rel_num == 0)
  {
    #ifdef deb
      printf("created sub relation in first slot of relation\n");
    #endif
    array[0] = create_sub_relation(src, rel, dest);
    rel->sub_relation_number ++;
    return 1;
  }
  else
  {
    for (i=0; i<rel_num; i++)
    {
      pointer = array[i];
      #ifdef deb
        printf("pointer: %d\n", pointer);
        printf("sub-relation is valid\n");
      #endif
      if (pointer != NULL)
      {
        if (pointer->source == src && pointer->destination == dest)
        {
            #ifdef deb
              printf("Relation already exists.\n");
            #endif
            return 0;
          }
          else
          {
              if (strcmp(dest->name, (pointer->destination)->name) < 0)
              {
                #ifdef deb
                printf("dest is smaller\n");
                #endif
                sub_rel = create_sub_relation(src, rel, dest);
                rel->sub_relation_number ++;
                sub_rel_array_fixup(array, i, rel_num+1, sub_rel);
                #ifdef deb
                  printf("added sub relation in first slot of relation\n");
                #endif
                return 1;
            }
          }
        }
      }
      rel->sub_relation_number ++;
      sub_rel = create_sub_relation(src, rel, dest);
      array[i] = sub_rel;
      #ifdef deb
        printf("created sub relation in last slot of relation\n");
      #endif
  }
  return 1;
}
//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

int handle_entity_creation(String name)
{
  int pos = entity_hash_function(name);
  if (entity_table[pos][0] == NULL)
  {
    entity_table[pos][0] = create_entity(name, pos);
    #ifdef deb
      printf("Created new entity in empty slot\n");
    #endif
    return 1;
  }
  else
  {
    int i;
    for(i=0; i<COLLISION_BUFFER_SIZE; i++)
    {
      if (entity_table[pos][i] == NULL)
      {
        #ifdef deb
          printf("Created new entity\n");
        #endif
        entity_table[pos][i] = create_entity(name, pos);
        return 1;
      }
      if (entity_table[pos][i]->valid == 0)
      {
        reallocate_entity(entity_table[pos][i], name);
        #ifdef deb
          printf("Reallocated entity\n");
        #endif
        return 1;
      }
      if (strcmp(name, entity_table[pos][i]->name) == 0)
      {
        #ifdef deb
          printf("Entity already exists\n");
        #endif
        return 0;
      }
    }
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle entity creation, return the created entity if success, else 0

int handle_relation_creation(String name1, String name2, String name)
{
  int pos = relation_hash_function(name);
  Entity* e1 = get_entity(name1);
  Entity* e2 = get_entity(name2);

  if (e1 == 0 || e2 == 0) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  if (relations_table[pos][0] == NULL)
  {
    relations_table[pos][0] = create_relation(name, pos, 0);
    #ifdef deb
      printf("Created new relation in empty table slot\n");
    #endif
    return relation_add_entities(relations_table[pos][0], e1, e2, pos);
  }
  else
  {
    #ifdef deb
      printf("first relation slot isn't empty\n");
    #endif
    int i;
    for(i=0; i<COLLISION_BUFFER_SIZE; i++)
    {
      #ifdef deb
        printf("rel: %s\n", relations_table[pos][i]->name);
      #endif
      if (relations_table[pos][i] == NULL)
      {
        relations_table[pos][i] = create_relation(name, pos, i);
        return relation_add_entities(relations_table[pos][i], e1, e2, pos);
      }
      if (strcmp(name, relations_table[pos][i]->name) == 0)
      {
        return relation_add_entities(relations_table[pos][i], e1, e2, pos);
      }
    }
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

int delent_function(String name)
{
  Entity* entity = get_entity(name);
  if (entity == 0)
  {
    #ifdef deb
      printf("invalid entity.\n");
    #endif
    return 0;
  }
  deallocate_entity(entity);
  #ifdef deb
    printf("deallocated entity at %x\n", entity);
  #endif
  return 1;
}
//deallocates an entity; return 1 if succes, else 0

int delrel_function(String name_source, String name_dest, String rel_name)
{
  Relation* rel = get_relation(rel_name);
  Entity* src = get_entity(name_source);
  Entity* dest = get_entity(name_dest);
  SubRelation* sub_rel;
  int i, rel_num;
  SubRelation** array;

  if (rel == 0 || src == 0 || dest == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("relation does not exist.\n");
    #endif
    return 0;
  }

  //delete sub relation from relation list in relation
  rel_num = rel->sub_relation_number;
  array = (SubRelation**) rel->sub_rel_array;
  for (i=0; i<rel_num; i++)
  {
    if (array[i] != NULL)
    {
      sub_rel = array[i];
      if (sub_rel->source == src && sub_rel->destination == dest)
      {
        free(sub_rel);
        rel->sub_relation_number --;
        sub_rel_array_fixup_delete(array, i, rel->sub_relation_number);
        //array[i]= NULL;
        #ifdef deb
          printf("deleted relation at i = %d\n", i);
        #endif
        return 1;
      }
    }
  }
  #ifdef deb
    printf("relation does not exist (LATE)\n");
  #endif
  return 0;
}
//deletes a relation; return 1 if success, else 0

void delete_unused_relation(Relation* rel)
{
  int hash = rel->hash_value;
  int pos = rel->collision_pos;
  //relations_table[hash][pos] = NULL;
  number_of_relations --;
  do {
    relations_table[hash][pos] = relations_table[hash][pos+1];
    pos ++;
  } while (relations_table[hash][pos] != NULL);
  rel_buffer_fixup_delete(rel->position);
  free(rel);
}
//returns the index of the next relation.

int report_function(int update)
{
  //if there are no relations, do nothing
  if (number_of_relations == 0)
  {
    printf("none\n");
    return 0;
  }

  static SuperLongString report_string;

  if (update == 1) //if the report needs to be updated:
  {
    //report string
    strcpy(report_string, "");

    //aux variables
    Entity* prev_entity;
    Entity* temp_entity;
    Relation* relation;
    SubRelation* sub_rel;
    SubRelation** array;
    char tmp_string[20];
    int i = 0, j, k, temp_counter = 0;
    int maximum;
    int rel_num;
    int nulls;

  //get maximums and load entities in 2d array
  for (int h=0; h<number_of_relations; h++)
  {
      relation = relations_buffer[h];
      prev_entity = NULL;
      rel_num = relation->sub_relation_number;
      if (rel_num > 0)
      {
        #ifdef deb
          printf("relation %s is valid, accessing sub-relations\n", relation->name);
        #endif
        maximums[i] = j = temp_counter = 0;
        array = (SubRelation**) relation->sub_rel_array;
        nulls = 0;
        for (k=0; k<rel_num; k++)
        {
          sub_rel = array[k];
          if (check_relation_validity(sub_rel) == 1)
          {
            temp_entity = sub_rel->destination;
            #ifdef deb
              printf("prev: %s, current: %s\n", prev_entity->name, temp_entity->name);
            #endif
            if (temp_entity == prev_entity)
            {
              #ifdef deb
                printf("increasing temp_counter\n");
              #endif
              temp_counter ++;
            }
            else
            {
              if (prev_entity != NULL)
              {
                if (temp_counter > maximums[i])
                  maximums[i] = temp_counter;
                entities_array[i][j].entity = prev_entity;
                entities_array[i][j].occurrences = temp_counter;
                #ifdef deb
                  printf("%d|%d: %s, %d;\n", i, j, prev_entity->name, temp_counter);
                #endif
                j ++;
                prev_entity = temp_entity;
                temp_counter = 1;
                #ifdef deb
                  printf("found a new entity, %s\n", temp_entity->name);
                #endif
              }
              else
              {
                prev_entity = temp_entity;
                temp_counter = 1;
                #ifdef deb
                  printf("found first entity\n");
                #endif
              }
            }
          }
          else
          {
            if (sub_rel != NULL)
            {
              free(sub_rel);
              array[k] = NULL;
            }
            nulls ++;
            #ifdef deb
              printf("nulls: %d\n", nulls);
            #endif
          }
        }

        if (nulls > 0)
        {
          relation->sub_relation_number -= nulls;
          sub_rel_array_fixup_delete_optimized(array, relation->sub_relation_number);
        }

        if (temp_counter > maximums[i])
          maximums[i] = temp_counter;
        entities_array[i][j].entity = prev_entity;
        entities_array[i][j].occurrences = temp_counter;
        entities_array[i][j+1].occurrences = 0;

        if (maximums[i] > 0)
        {
          relations_array[i] = relation;
          i ++;
          #ifdef deb
            printf("maximum is > 0, added relation to report array.\n");
          #endif
        }
        else
        {
          #ifdef deb
            printf("relation %s is unused, deleting(LATE)\n", relation->name);
          #endif
          delete_unused_relation(relation);
          h --;
        }
    }
    else
    {
      #ifdef deb
        printf("relation %s is unused, deleting\n", relation->name);
      #endif
      delete_unused_relation(relation);
      h --;
    }
  }

  relations_array[i] = NULL;

  #ifdef deb
    printf("finished phase one, printing...\n");
  #endif

  if (relations_array[0] == NULL)
  {
    #ifdef deb
      printf("(LATE)\n");
    #endif
    printf("none\n");
    return 0;
  }

  for (i=0; relations_array[i] != NULL; i++)
  {
      maximum = maximums[i];
      strcat(report_string, relations_array[i]->name);
      strcat(report_string, " ");
      for (j=0; j<number_of_entities; j++)
      {
        if (entities_array[i][j].occurrences == 0)
          break;
        if (entities_array[i][j].occurrences == maximum)
        {
          strcat(report_string, (entities_array[i][j].entity)->name);
          strcat(report_string, " ");
        }
      }
      sprintf(tmp_string, "%d; ", maximum);
      strcat(report_string, tmp_string);
  }

  }

  printf("%s\n", report_string);
  return 1;
}

//debug functions---------------------------------------------------------------

#ifdef deb

void deb_print_entities()
{
  Entity* entity;
  for (int i=0; i<ENTITY_TABLE_SIZE; i++)
  {
    if (entity_table[i][0] != NULL)
    {
        printf("%d: ", i);
        for (int j=0; j<COLLISION_BUFFER_SIZE; j++)
        {
          entity = entity_table[i][j];
          if (entity == NULL)
            break;
          if (entity->valid == 1)
            printf("%s [id = %d], ", entity->name, entity->entity_id);
          else
            printf("(deallocated) [id = %d], ", entity->entity_id);
        }
        printf("\n");
    }
  }
}

void deb_print_sub_relations(Relation* rel)
{
  int i;
  int rel_num = rel->sub_relation_number;
  SubRelation** array = rel->sub_rel_array;
  for(i=0; i<rel_num; i++)
  {
      if (array[i] != NULL)
      {
        Entity* src = array[i]->source;
        Entity* dest = array[i]->destination;
        printf("%d: %s --%s--> %s\n", i, src->name, rel->name, dest->name);
      }
      else
        printf("%d: NULLED\n", i);
  }
}

void deb_print_relations()
{
  for (int i=0; i<number_of_relations; i++)
  {
    deb_print_sub_relations(relations_buffer[i]);
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
//check the first 6 characters of the input string and generate
//the correct opcode.

//------------------------------------------------------------------------------
int main() //main program
{
  String argument0;
  String argument1;
  String argument2;
  LongString input_string, prev_command;
  int report_needed = 1; // 0 = report has not changed, 1 = report has changed.
  #ifdef deb
    int command_counter = 0;
  #endif

  int opcode, prev_opcode;
  while (1)
  {
    fgets(input_string, 160, stdin);
    opcode = generate_opcode(input_string);

    if (prev_opcode == opcode)
    {
      if (opcode != 4)
      {
        if (strcmp(prev_command, input_string) == 0)
          opcode = 6;
      }
    }

    prev_opcode = opcode;
    strcpy(prev_command, input_string);

    #ifdef deb
      printf("\nCOMMAND %d: %s\n", command_counter, input_string);
      command_counter ++;
    #endif

    switch (opcode)
    {
      case 0: //addent
      {
        get_argument(input_string, argument0, 7);
        handle_entity_creation(argument0);
        #ifdef deb
          deb_print_entities();
          deb_print_relations();
        #endif
        break;
      }
      case 1: //addrel
      {
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);
        if (handle_relation_creation(argument0, argument1, argument2) != 0)
          report_needed = 1;
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 2: //delent
      {
        get_argument(input_string, argument0, 7);
        if (delent_function(argument0) != 0)
          report_needed = 1;
        #ifdef deb
          deb_print_entities();
          deb_print_relations();
        #endif
        break;
      }
      case 3: //delrel
      {
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);
        if (delrel_function(argument0, argument1, argument2) != 0)
          report_needed = 1;
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 4: //report
      {
        if (report_function(report_needed) != 0)
          report_needed = 0;
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 5: //end
      {
        return 0;
        break;
      }
      case 6:
      {
        #ifdef deb
          printf("same command\n");
        #endif
        break;
      }
    }
  }
}
