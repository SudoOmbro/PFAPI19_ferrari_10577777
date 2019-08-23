#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  TODO:
  - cambia le sotto relazioni in 2 array che contengono src e dest.
  - metti a posto il report in modo che funzioni e faccia meno accessi.
  (- pulisci il deleted_entities_array.)
*/

#define ENTITY_TABLE_SIZE  6451 //6451
#define SUB_RELATIONS_ARRAY_SIZE 98304 //78024
#define RELATIONS_BUFFER_SIZE 4096
#define COLLISION_BUFFER_SIZE 21 //32

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char SuperLongString[5632];
typedef char LongString[128];
typedef char String[32];

typedef struct {
  String name;
  int sub_relation_number;
  void* sub_rel_array[SUB_RELATIONS_ARRAY_SIZE];
} Relation;

typedef struct {
  String name;
  unsigned long value;
  int rel_num;
} Entity;

typedef struct {
  Entity* source;
  Entity* destination;
  String dest_name;
} SubRelation;  //the relation is the relation that contains it.

int number_of_relations = 0;

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

unsigned long entity_hash_function(char* text)
{
  unsigned long value = 0;
  for (int i=1; text[i] != '"'; i++)
  {
    value = 131*value + text[i];
  }
  return value;
}

int get_argument(LongString input_string, char* dest_string, int start_pos)
{
  char byte = input_string[start_pos];
  dest_string[0] = '"';
  int string_length = 1;
  while (byte != '"') //while the char is valid
  {
    dest_string[string_length] = byte;
    string_length ++;
    start_pos ++;
    byte = input_string[start_pos];
  }
  dest_string[string_length] = '"';
  dest_string[string_length+1] = '\0';
  return start_pos;
}
//given a string in stdin, it fills dest_string with the input_string
//until a ' " ' is found. Returns position in input string.

Entity* entity_hash_table_linear_search(Entity* array[COLLISION_BUFFER_SIZE], unsigned long value)
{
  #ifdef deb
    printf("starting linear search\n");
  #endif
  for(int i=0; array[i] != NULL; i++)
  {
    if (value == array[i]->value)
    {
      #ifdef deb
        printf("linear search done, found entity\n");
      #endif
      return array[i];
    }
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
  return 0;
}
//looks for entity in a specified collision buffer, returns the entity pointer.

int entity_hash_table_linear_search_pos(Entity* array[COLLISION_BUFFER_SIZE], unsigned long value)
{
  #ifdef deb
    printf("starting linear search\n");
  #endif
  for(int i=0; array[i] != NULL; i++)
  {
    if (value == array[i]->value)
    {
      #ifdef deb
        printf("linear search done, found entity\n");
      #endif
      return i;
    }
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
  return -1;
}
//looks for entity in a specified collision buffer, returns postition in buffer.

Entity* get_entity(Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], String name)
{
  #ifdef deb
    printf("getting entity %s.\n", name);
  #endif
  unsigned long value = entity_hash_function(name);
  int pos = value % ENTITY_TABLE_SIZE;
  if (entity_table[pos][0] != NULL)
    return entity_hash_table_linear_search(entity_table[pos], value);
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

int relations_binary_search(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name, int l, int r)
{
  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        int mid = l + (r - l) / 2;
        int val = strcmp(name, relations_buffer[mid]->name);
        if (val == 0)
        {
            #ifdef deb
              printf("found relation %s.\n", name);
            #endif
            return mid;
        }
        if (val < 0)
            r = mid - 1;
        else
            l = mid + 1;
  }
  #ifdef deb
    printf("no match found for %s.\n", name);
  #endif
  return -1;
}

int sub_relations_binary_search(SubRelation** array, Entity* dest, int l, int r, Entity* src)
{
  while (r >= l)
  {
        #ifdef deb
          printf("sub rel binary search %d, %d\n", l, r);
        #endif
        int mid = l + (r - l) / 2;
        if (array[mid]->destination == dest)
        {
            #ifdef deb
              printf("checking entity %s.\n", (array[mid]->dest_name));
            #endif
            Entity* val = array[mid]->source;
            if (val == src)
            {
              return mid;
            }
            if (val > src)
                r = mid - 1;
            else
                l = mid + 1;
        }
        else
        {
          #ifdef deb
            printf("nope.\n");
          #endif
          if (array[mid]->destination < dest)
            r = mid - 1;
          else
            l = mid + 1;
        }
  }
  #ifdef deb
    printf("r(%d) < l(%d)\n", r, l);
  #endif
  return -1;
}

int sub_relations_binary_search_creation(SubRelation** array, Entity* dest, int l, int r, Entity* src)
{
  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        int mid = l + (r - l) / 2;
        if (array[mid]->destination == dest)
        {
            Entity* val = array[mid]->source;
            if (val == src)
            {
              #ifdef deb
                printf("relation already exists.\n");
              #endif
              return -1;
            }
            if (val < src)
              l = mid + 1;
            else
              r = mid - 1;
        }
        else if (array[mid]->destination < dest)
        {
            #ifdef deb
              printf("int %s < %s\n", dest->name, (array[mid]->dest_name));
            #endif
            r = mid - 1;
        }
        else
        {
          #ifdef deb
            printf("int %s > %s\n", dest->name, (array[mid]->dest_name));
          #endif
          l = mid + 1;
        }
  }
  #ifdef deb
    printf("r(%d) < l(%d)\n", r, l);
  #endif
  return l;
}

Relation* get_relation(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name)
{
  #ifdef deb
    printf("getting relation %s.\n", name);
  #endif
  int rel_num = number_of_relations;

  if (rel_num == 0)
    return 0;

  int val = relations_binary_search(relations_buffer, name, 0, rel_num-1);
  if (val != -1)
    return relations_buffer[val];
  else
    return 0;
}

Entity* create_entity(String name, unsigned long value)
{
  //create the entity in the hash table
  Entity* self;
  self = (Entity*) malloc(sizeof(Entity));
  strcpy(self->name, name);
  self->value = value;
  self->rel_num = 0;
  return self;
}
//crea entità, assegna il nome e ritorna il puntatore.

void sub_rel_array_fixup(SubRelation** array, int start_pos, const int max, SubRelation* replacer)
{
  int i;
  for (i=max; i>start_pos; i--)
    array[i] = array[i-1];
  array[i] = replacer;
}
//given the pointer to the array to restore, the starting position, the
//end postion and the entity that replaces, this function restores
//the order in the array.

void rel_buffer_fixup(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int start_pos, Relation* replacer)
{
  int i;
  for (i=number_of_relations; i>start_pos; i--)
    relations_buffer[i] = relations_buffer[i-1];
  relations_buffer[i] = replacer;
}

void sub_rel_array_fixup_delete(SubRelation** array, int start_pos, int end)
{
  int i;
  for (i=start_pos; i<end; i++)
    array[i] = array[i+1];
  array[i] = NULL;
}
//given the pointer to the array to restore, the starting position
//and the end postion, this function restores the order in the array.

void rel_buffer_fixup_delete(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int start_pos)
{
  int i;
  int rel_num = number_of_relations;
  for (i=start_pos; i<rel_num; i++)
    relations_buffer[i] = relations_buffer[i+1];
  relations_buffer[i] = NULL;
}

Relation* create_relation(String name)
{
  //create the relation to put in hash table
  Relation* self = (Relation*) malloc(sizeof(Relation));
  memcpy(self->name, name, sizeof(String));
  self->sub_relation_number = 0;
  return self;
}
//Create relation in hash table, create relation in ordered list and
//return the pointer to the relation in hash table.

SubRelation* create_sub_relation(Entity* src, Entity* dest)
{
  #ifdef deb
    printf("creating sub-relation...\n");
  #endif
  SubRelation* self = (SubRelation*) malloc(sizeof(SubRelation));
  self->source = src;
  self->destination = dest;
  memcpy(self->dest_name, dest->name, sizeof(String));
  src->rel_num ++;
  dest->rel_num ++;
  return self;
}
//creates a sub relation and returns the pointer to that.

int relation_add_entities(Relation* rel, Entity* src, Entity* dest)
{
  //setup
  int rel_num = rel->sub_relation_number;
  SubRelation* sub_rel;
  SubRelation** array = (SubRelation**) rel->sub_rel_array;
  if (rel_num == 0)
  {
    #ifdef deb
      printf("created sub relation in first slot of relation\n");
    #endif
    array[0] = create_sub_relation(src, dest);
    rel->sub_relation_number ++;
    return 1;
  }
  else
  {
    int mid_val = sub_relations_binary_search_creation(array, dest, 0, rel_num-1, src);
    if (mid_val > -1)
    {
      #ifdef deb
        printf("found a spot\n");
      #endif
      sub_rel = create_sub_relation(src, dest);
      rel_num ++;
      sub_rel_array_fixup(array, mid_val, rel_num, sub_rel);
      rel->sub_relation_number = rel_num;
      #ifdef deb
        printf("added sub relation in %d\n", mid_val);
      #endif
      return 1;
    }
    return 0;
  }
}
//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

void entity_array_fixup(Entity* array[COLLISION_BUFFER_SIZE], int start, Entity* replacer)
{
  int i;
  Entity* temp;
  for (i=start; array[i] != NULL; i++)
  {
    temp = array[i];
    array[i] = replacer;
    replacer = temp;
  }
  array[i] = replacer;
}

void entity_array_fixup_delete(Entity* array[COLLISION_BUFFER_SIZE], int start)
{
  int i;
  for (i=start; array[i+1] != NULL; i++)
    array[i] = array[i+1];
  array[i] = NULL;
}

int handle_entity_creation(Entity* entity_table[][COLLISION_BUFFER_SIZE], String name)
{
  //do the hash function but also save the value of the entity
  unsigned long value = 0;
  for (int i=1; name[i] != '"'; i++)
  {
    value = 131*value + name[i];
  }

  #ifdef deb
    printf("entity value: %lu\n", value);
    printf("entity hash value: %d\n", (value) % ENTITY_TABLE_SIZE);
  #endif
  int pos = (value) % ENTITY_TABLE_SIZE;

    int i;
    for(i=0; entity_table[pos][i] != NULL; i++)
    {
      if (value == entity_table[pos][i]->value)
      {
        #ifdef deb
          printf("Entity already exists\n");
        #endif
        return 0;
      }
      else if (strcmp(entity_table[pos][i]->name, name) < 0)
      {
        Entity* entity = create_entity(name, value);
        #ifdef deb
          printf("Created new entity\n");
        #endif
        entity_array_fixup(entity_table[pos], i, entity);
        return 1;
      }
    }

    entity_table[pos][i] = create_entity(name, value);
    #ifdef deb
      printf("Created new entity\n");
    #endif
    return 1;
}
//handle entity creation, return the created entity if success, else 0

int handle_relation_creation(Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name1, String name2, String name)
{
  Entity* e1 = get_entity(entity_table, name1);
  Entity* e2 = get_entity(entity_table, name2);

  if (e1 == 0 || e2 == 0) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  #ifdef deb
    printf("all of the entities exist\n");
  #endif

  static String prev_relation_name = "";
  static Relation* prev_rel = 0;

  if (strcmp(prev_relation_name, name) == 0)
    return relation_add_entities(prev_rel, e1, e2);

  int rel_num = number_of_relations;

  if (rel_num == 0)
  {
    relations_buffer[0] = create_relation(name);
    number_of_relations ++;
    return relation_add_entities(relations_buffer[0], e1, e2);
  }

  int r = rel_num-1, l = 0, val, mid;

  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        mid = l + (r - l) / 2;
        val = strcmp(name, relations_buffer[mid]->name);
        if (val == 0)
        {
            #ifdef deb
              printf("found relation %s.\n", name);
            #endif
            prev_rel = relations_buffer[mid];
            memcpy(prev_relation_name, name, sizeof(String));
            return relation_add_entities(relations_buffer[mid], e1, e2);
        }
        if (val < 0)
            r = mid - 1;
        else
            l = mid + 1;
  }

  #ifdef deb
    printf("created new relation in %d", l);
  #endif

  Relation* self = create_relation(name);
  prev_rel = self;
  memcpy(prev_relation_name, name, sizeof(String));
  number_of_relations ++;
  rel_buffer_fixup(relations_buffer, l, self);
  return relation_add_entities(self, e1, e2);
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

Entity* delent_function(Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], String name)
{
  unsigned long value = entity_hash_function(name);

  int pos = value % ENTITY_TABLE_SIZE;
  int entity_pos = entity_hash_table_linear_search_pos(entity_table[pos], value);

  if (entity_pos == -1)
  {
    #ifdef deb
      printf("invalid entity.\n");
    #endif
    return 0;
  }
  Entity* entity = entity_table[pos][entity_pos];
  entity_array_fixup_delete(entity_table[pos], entity_pos);

  #ifdef deb
    printf("deallocated entity at %p\n", (void*) entity);
  #endif
  if (entity->rel_num != 0)
    return entity;
  return 0;
}
//deallocates an entity; return the entity pointer if succes, else 0

void delete_unused_relation(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], Relation* rel, int start_pos)
{
  number_of_relations --;
  rel_buffer_fixup_delete(relations_buffer, start_pos);
  free(rel);
}

int delrel_function(Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name_source, String name_dest, String rel_name)
{
  //static variables for speedy checking
  static Relation* prev_rel = 0;
  static String prev_rel_name = "";

  //get relation pointer
  Relation* rel;
  if (strcmp(prev_rel_name, rel_name) == 0)
  {
    #ifdef deb
      printf("same as previous relation (%s)\n", prev_rel_name);
    #endif
    if (prev_rel->sub_relation_number == 0)
    {
      #ifdef deb
        printf("relation %s is empty, exiting...\n", prev_rel_name);
      #endif
      return 0;
    }
    rel = prev_rel;
  }
  else
  {
    rel = get_relation(relations_buffer, rel_name);
    if (rel == 0)
    {
      #ifdef deb
        printf("relation does not exist.\n");
      #endif
      return 0;
    }
    prev_rel = rel;
    memcpy(prev_rel_name, rel_name, sizeof(String));
  }

  Entity* src = get_entity(entity_table, name_source);
  Entity* dest = get_entity(entity_table, name_dest);

  if (src == 0 || dest == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("one of the entities does not exist.\n");
    #endif
    return 0;
  }

  SubRelation* sub_rel;
  int i;
  SubRelation** array;

  //delete sub relation from relation list in relation
  #ifdef deb
    printf("relation may exist.\n");
  #endif
  array = (SubRelation**) rel->sub_rel_array;
  i = sub_relations_binary_search(array, dest, 0, rel->sub_relation_number-1, src);
  if (i > -1)
  {
      sub_rel = array[i];
      free(sub_rel);
      rel->sub_relation_number --;
      src->rel_num --;
      dest->rel_num --;
      sub_rel_array_fixup_delete(array, i, rel->sub_relation_number);
      #ifdef deb
        printf("deleted relation at i = %d\n", i);
      #endif
      return 1;
  }
  #ifdef deb
    printf("relation does not exist (LATE)\n");
  #endif
  return 0;
}
//deletes a relation; return 1 if success, else 0

int check_sub_rel_validity(SubRelation* sub_rel, Entity** del_array, int del_num)
{
  Entity* dest = sub_rel->destination;
  Entity* src = sub_rel->source;
  int tmp = 0;
  for (int i=0; i<del_num; i++)
  {
    tmp += (dest == del_array[i]);
    tmp += (src == del_array[i]);
    if (tmp != 0)
      return 0;
  }
  return 1;
}

int check_entity_validity(Entity* entity, Entity** array, int del_num)
{
  for (int i=0; i<del_num; i++)
  {
    if (entity == array[i])
      return 0;
  }
  return 1;
}

int report_function(Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int update, Entity** del_array, int del_num)
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
    SuperLongString entities_string;
    memcpy(report_string, "", 1);

    //aux variables
    Entity* prev_entity;
    Entity* temp_entity;
    Relation* relation;
    SubRelation* sub_rel;
    SubRelation* prev_rel;
    SubRelation** array;
    char tmp[20];
    int i = 0, k, temp_counter = 0;
    int maximum;
    int rel_num;
    int rr = number_of_relations;

  if (del_num > 0) {

  #ifdef deb
    printf("entities have been deleted.\n");
  #endif

  for (int h=0; h<rr; h++)
  {
      relation = relations_buffer[h];
      rel_num = relation->sub_relation_number;
      if (rel_num > 0)
      {
        #ifdef deb
          printf("relation %s is valid, accessing sub-relations\n", relation->name);
        #endif
        array = (SubRelation**) relation->sub_rel_array;
        prev_rel = array[0];
        prev_entity = 0;
        temp_counter = maximum = 0;
        for (k=0; k<rel_num; k++)
        {
          #ifdef deb
            printf("%d\n", k);
          #endif
          sub_rel = array[k];
          temp_entity = sub_rel->destination;
          if (prev_entity == temp_entity)
          {
            if (check_entity_validity(sub_rel->source, del_array, del_num) == 1)
            {
              #ifdef deb
                printf("prev: %s, current: %s\n", prev_entity->name, temp_entity->name);
              #endif
              temp_counter ++;
            }
            else
            {
              free(array[k]);
              relation->sub_relation_number --;
              rel_num --;
              sub_rel_array_fixup_delete(array, k, rel_num);
              k--;
              #ifdef deb
                printf("deleted relation at position %d\n", k);
              #endif
            }
          }
          else
          {
            if (temp_counter > maximum)
            {
              maximum = temp_counter;
              strcpy(entities_string, prev_rel->dest_name);
              strcat(entities_string, " ");
            }
            else if (temp_counter == maximum)
            {
              strcat(entities_string, prev_rel->dest_name);
              strcat(entities_string, " ");
            }

            if (check_entity_validity(sub_rel->destination, del_array, del_num) == 1)
            {
              #ifdef deb
                printf("%d: %s, %d;\n", i, prev_entity->name, temp_counter);
              #endif
              prev_entity = temp_entity;
              prev_rel = sub_rel;
              temp_counter = 1;
              #ifdef deb
                printf("found a new entity, %s\n", temp_entity->name);
              #endif
            }
            else
            {
              free(array[k]);
              relation->sub_relation_number --;
              rel_num --;
              sub_rel_array_fixup_delete(array, k, rel_num);
              k--;
              #ifdef deb
                printf("deleted relation at position %d\n", k);
              #endif
              prev_rel = 0;
              prev_entity = 0;
            }
        }
    }
  }
  else
  {
  #ifdef deb
    printf("relation %s is unused, deleting\n", relation->name);
  #endif
    delete_unused_relation(relations_buffer, relation, h);
    rr --;
    h --;
  }
}

} else {

  #ifdef deb
    printf("entities have NOT been deleted.\n");
  #endif

  for (int h=0; h<rr; h++)
  {
      relation = relations_buffer[h];
      rel_num = relation->sub_relation_number;
      if (rel_num > 0)
      {
        #ifdef deb
          printf("relation %s is valid, accessing sub-relations\n", relation->name);
        #endif
        temp_counter = maximum = 0;
        array = (SubRelation**) relation->sub_rel_array;
        prev_rel = array[0];
        prev_entity = prev_rel->destination;
        for (k=0; k<rel_num; k++)
        {
            sub_rel = array[k];
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
                if (temp_counter > maximum)
                {
                  maximum = temp_counter;
                  strcpy(entities_string, prev_rel->dest_name);
                  strcat(entities_string, " ");
                }
                else if (temp_counter == maximum)
                {
                  strcat(entities_string, prev_rel->dest_name);
                  strcat(entities_string, " ");
                }
                #ifdef deb
                  printf("%d: %s, %d;\n", i, prev_rel->dest_name, temp_counter);
                #endif
                prev_entity = temp_entity;
                prev_rel = sub_rel;
                temp_counter = 1;
                #ifdef deb
                  printf("found a new entity, %s\n", temp_entity->name);
                #endif
            }
          }
        }

        if (temp_counter > maximum)
        {
          maximum = temp_counter;
          strcpy(entities_string, prev_rel->dest_name);
          strcat(entities_string, " ");
        }
        else if (temp_counter != 0 && temp_counter == maximum)
        {
          strcat(entities_string, prev_rel->dest_name);
          strcat(entities_string, " ");
        }

        if (maximum > 0)
        {
          strcat(report_string, relation->name);
          strcat(report_string, " ");
          strcat(report_string, entities_string);
          sprintf(tmp, "%d; ", maximum);
          strcat(report_string, tmp);
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
          delete_unused_relation(relations_buffer, relation, h);
          rr--;
          h --;
        }
    }

  }
  #ifdef deb
    printf("finished phase one, printing...\n");
  #endif

  }

  printf("%s\n", report_string);
  return 1;
}

//debug functions---------------------------------------------------------------

#ifdef deb

void deb_print_entities(Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE])
{
  Entity* entity;
  for (int j=0; j<ENTITY_TABLE_SIZE; j++)
  {
    if (entity_table[j][0] != NULL)
    {
      for (int i=0; entity_table[j][i] != NULL; i++)
      {
        entity = entity_table[j][i];
        printf("%s, ", entity->name);
      }
      printf("\n");
    }
  }
}

void deb_print_sub_relations(Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], Relation* rel)
{
  int i;
  int rel_num = rel->sub_relation_number;
  SubRelation** array = (SubRelation**) rel->sub_rel_array;
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

void deb_print_relations(Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE], Relation* relations_buffer[RELATIONS_BUFFER_SIZE])
{
  for (int i=0; i<number_of_relations; i++)
  {
    if (relations_buffer[i]->sub_relation_number > 0)
      deb_print_sub_relations(entity_table, relations_buffer[i]);
  }
}

#endif

//------------------------------------------------------------------------------
int main() //main program
{
  //main data structures
  Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE] = {NULL};
  Entity* deleted_entities_array[1024];
  int del_ent_num = 0;
  Relation* relations_buffer[RELATIONS_BUFFER_SIZE];
  //aux variables
  String argument0;
  String argument1;
  String argument2;
  LongString input_string;
  int prev_opcode = 5;
  String prev_arg = "";
  int report_needed = 1; // 0 = report has not changed, 1 = report has changed.
  #ifdef deb
    int command_counter = 0;
  #endif

  int opcode;
  while (1)
  {
    fgets(input_string, 160, stdin);

    #ifdef deb
      printf("\nCOMMAND %d: %sPREVIOUS ARG: %s\nPREVIOUS OPCODE: %d\n\n", command_counter, input_string, prev_arg, prev_opcode);
      command_counter ++;
    #endif

    if (input_string[0] == 'a') //addent or addrel
    {
      if (input_string[3] == 'e') //addent
      {
        get_argument(input_string, argument0, 8);
        if (prev_opcode != 0)
        {
          handle_entity_creation(entity_table, argument0);
          prev_opcode = 0;
        }
        else
        {
          if (strcmp(prev_arg, argument0) != 0)
          {
            handle_entity_creation(entity_table, argument0);
          }
        }
        memcpy(prev_arg, argument0, sizeof(String));
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      else //addrel
      {
        opcode = get_argument(input_string, argument0, 8);
        opcode = get_argument(input_string, argument1, opcode+3);
        get_argument(input_string, argument2, opcode+3);
        if (handle_relation_creation(entity_table, relations_buffer,  argument0, argument1, argument2) != 0)
        {
          report_needed = 1;
          prev_opcode = 1;
        }
        #ifdef deb
          //deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
    }
    else if (input_string[0] == 'd') //delent or delrel
    {
      if (input_string[3] == 'e') //delent
      {
        get_argument(input_string, argument0, 8);
        if (prev_opcode != 2)
        {
          Entity* ent = delent_function(entity_table, argument0);
          if (ent != 0)
          {
            deleted_entities_array[del_ent_num] = ent;
            del_ent_num ++;
            report_needed = 1;
          }
          prev_opcode = 2;
        }
        else
        {
          if (strcmp(prev_arg, argument0) != 0)
          {
            Entity* ent = delent_function(entity_table, argument0);
            if (ent != 0)
            {
              deleted_entities_array[del_ent_num] = ent;
              del_ent_num ++;
              report_needed = 1;
            }
          }
        }
        memcpy(prev_arg, argument0, sizeof(String));
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      else //delrel
      {
        opcode = get_argument(input_string, argument0, 8);
        opcode = get_argument(input_string, argument1, opcode+3);
        get_argument(input_string, argument2, opcode+3);
        if (delrel_function(entity_table, relations_buffer, argument0, argument1, argument2) != 0)
        {
          report_needed = 1;
          prev_opcode = 3;
        }
        #ifdef deb
          //deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
    }
    else if (input_string[0] == 'r') //report
    {
      report_function(entity_table, relations_buffer, report_needed, deleted_entities_array, del_ent_num);
      del_ent_num = 0;
      report_needed = 0;
      #ifdef deb
        //deb_print_entities(entity_table);
        deb_print_relations(entity_table, relations_buffer);
      #endif
    }
    else
    {
      #ifdef deb
        printf("execution ended correctly.\n");
      #endif
      return 0;
    }
  }
}
