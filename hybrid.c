#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITY_TABLE_SIZE  711353 //6451
#define SUB_RELATIONS_ARRAY_SIZE 98304 //78024
#define RELATIONS_BUFFER_SIZE 128
#define SRC_ARRAY_BUFFER 640

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
  unsigned long value;
  String name;
  int rel_num;
} Entity;

typedef struct {
  String dest_name;
  Entity* destination;
  int src_num;
  Entity* source_buffer[SRC_ARRAY_BUFFER];
} SubRelation;  //the relation is the relation that contains it.

int number_of_relations = 0;
#ifdef deb
  int collisions = 0;
#endif

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

inline __attribute__((always_inline)) unsigned long entity_hash_function(char* text)
{
  unsigned long hash = 0;
  int i = 1;

  while(text[i] != '"')
    hash = hash*131 + text[i++];

  #ifdef deb
    printf("entity hash: %lu\n", hash);
  #endif

  return hash;
}

char* get_argument(LongString input_string, char* dest_string)
{
  *dest_string = '"';
  while (*input_string != '"') //while the char is valid
    *++dest_string = *input_string++;
  *++dest_string = '"';
  *++dest_string = '\0';
  return input_string+3;
}
//given a string in stdin, it fills dest_string with the input_string
//until a ' " ' is found. Returns position in input string.

Entity* get_entity(Entity** entity_table, String name)
{
  #ifdef deb
    printf("getting entity %s.\n", name);
  #endif
  unsigned long value = entity_hash_function(name);
  int pos = value % ENTITY_TABLE_SIZE;
  if (entity_table[pos] != NULL)
    return entity_table[pos];
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

int relations_binary_search(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name, int l, int r)
{
  int mid, val;
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

int sub_relations_binary_search(SubRelation** array, String name, int l, int r, Entity* src)
{
  int mid;
  int out_val;
  while (r >= l)
  {
        #ifdef deb
          printf("sub rel binary search %d, %d\n", l, r);
        #endif
        mid = l + (r - l) / 2;
        out_val = strcmp(name, (array[mid]->dest_name));
        if (out_val == 0)
        {
          return mid;
        }
        else
        {
          #ifdef deb
            printf("nope.\n");
          #endif
          if (out_val < 0)
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
//looks for a sub relation, if it finds it it returns its postion, else
//it returns -1.

void sub_rel_array_fixup(void** array, int start_pos, const int max, void* replacer)
{
  int i;
  for (i=max; i>start_pos; i--)
    array[i] = array[i-1];
  array[i] = replacer;
}
//given the pointer to the array to restore, the starting position, the
//end postion and the entity that replaces, this function restores
//the order in the array.

void sub_rel_array_fixup_delete(void** array, int start_pos, int end)
{
  int i;
  for (i=start_pos; i<end; i++)
    array[i] = array[i+1];
  array[i] = NULL;
}
//given the pointer to the array to restore, the starting position
//and the end postion, this function restores the order in the array.

int sub_relations_binary_search_creation(SubRelation** array, String dest_name, int l, int r, Entity* src)
{
  int mid;
  int ext_val;
  while (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        mid = l + (r - l) / 2;
        ext_val = strcmp(dest_name, (array[mid]->dest_name));
        if (ext_val == 0)
        {
          #ifdef deb
            printf("Destination exists");
          #endif
          return mid;
        }
        else if (ext_val < 0)
        {
            #ifdef deb
              printf("int %s < %s\n", dest_name, (array[mid]->dest_name));
            #endif
            r = mid - 1;
        }
        else
        {
          #ifdef deb
            printf("int %s > %s\n", dest_name, (array[mid]->dest_name));
          #endif
          l = mid + 1;
        }
  }
  #ifdef deb
    printf("r(%d) < l(%d)\n", r, l);
  #endif
  return l;
}
//looks for a sub relation, if it finds it it returns its position,
//else it returns where to create it.

int src_binary_search(Entity** src_array, Entity* src, int l, int r)
{
  int mid;
  Entity* val;
  while (r >= l)
  {
    mid = l + (r - l) / 2;
    val = src_array[mid];
    if (val == src)
    {
      #ifdef deb
        printf("relation exists.\n");
      #endif
      return mid;
    }
    if (val < src)
      l = mid + 1;
    else
      r = mid - 1;
  }
  return -1;
}
//looks for an entity in the src_buffer of a relation, if it finds it it returns
//its position, else it returns -1.

int src_binary_search_creation(Entity** src_array, Entity* src, int l, int r)
{
  int size = r;
  int mid;
  Entity* val;
  while (r >= l)
  {
    mid = l + (r - l) / 2;
    val = src_array[mid];
    if (val == src)
    {
      #ifdef deb
        printf("relation already exists.\n");
      #endif
      return 0;
    }
    if (val < src)
      l = mid + 1;
    else
      r = mid - 1;
  }
  sub_rel_array_fixup((void**)src_array, l, r, src);
  return 1;
}
//looks for an entity in the src_buffer of a relation, if it finds it it returns
//-1, else it returns the position where to create it.

Relation* get_relation(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name)
{
  #ifdef deb
    printf("getting relation %s.\n", name);
  #endif

  if (number_of_relations == 0)
    return 0;

  int val = relations_binary_search(relations_buffer, name, 0, number_of_relations-1);
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
  self->rel_num = 0;
  self->value = value;
  return self;
}
//create entity, initialize values.

void rel_buffer_fixup(Relation** array, int start_pos, Relation* replacer)
{
  int i;
  for (i=number_of_relations; i>start_pos; i--)
    array[i] = array[i-1];
  array[i] = replacer;
}
//fix the relations array when addition.

void rel_buffer_fixup_delete(Relation** relations_buffer, int start_pos)
{
  int i;
  for (i=start_pos; i<number_of_relations; i++)
    relations_buffer[i] = relations_buffer[i+1];
  relations_buffer[i] = NULL;
}
//fix the relatiobs array when deletion.

Relation* create_relation(String name)
{
  //create the relation to put in hash table
  Relation* self = (Relation*) malloc(sizeof(Relation));
  strcpy(self->name, name);
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
  self->src_num = 1;
  *(self->source_buffer) = src;
  self->destination = dest;
  strcpy(self->dest_name, dest->name);
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
    int mid_val = sub_relations_binary_search_creation(array, dest->name, 0, rel_num-1, src);
    if (array[mid_val]->destination == dest)
    {
      SubRelation* sub_rel = array[mid_val];
      if (src_binary_search_creation(sub_rel->source_buffer, src, 0, sub_rel->src_num) == 0)
      {
        return 0;
      }
      else
      {
        ++(sub_rel->src_num);
        return 1;
      }
    }
    else
    {
      #ifdef deb
        printf("found a spot\n");
      #endif
      sub_rel = create_sub_relation(src, dest);
      ++ rel_num;
      sub_rel_array_fixup( (void**) array, mid_val, rel_num, sub_rel);
      rel->sub_relation_number = rel_num;
      #ifdef deb
        printf("added sub new destination in %d\n", mid_val);
      #endif
      return 1;
    }
    return 0;
  }
}
//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

int handle_entity_creation(Entity** entity_table, String name)
{
  //do the hash functio but also save the value of the entity
  unsigned long value = entity_hash_function(name);
  int pos = (value) % ENTITY_TABLE_SIZE;
  #ifdef deb
    printf("entity value: %d\n", value);
    printf("entity hash value: %d\n", (value) % ENTITY_TABLE_SIZE);
  #endif

  if (entity_table[pos] == NULL)
  {
    #ifdef deb
      printf("Created new entity\n");
    #endif
    entity_table[pos] = create_entity(name, value);
    return 1;
  }
  #ifdef deb
    printf("Entity already exists\n");
    if (value != entity_table[pos]->value)
      collisions ++;
  #endif
  return 0;
}
//handle entity creation, return the created entity if success, else 0

int handle_relation_creation(Entity** entity_table, Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name1, String name2, String name)
{
  Entity* e1 = get_entity(entity_table, name1);
  if (e1 == 0) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }
  Entity* e2 = get_entity(entity_table, name2);
  if (e2 == 0) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  #ifdef deb
    printf("all of the entities exist\n");
  #endif

  if (number_of_relations == 0)
  {
    relations_buffer[0] = create_relation(name);
    number_of_relations ++;
    return relation_add_entities(relations_buffer[0], e1, e2);
  }

  int r = number_of_relations-1, l = 0, val, mid;

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
  number_of_relations ++;
  rel_buffer_fixup(relations_buffer, l, self);
  return relation_add_entities(self, e1, e2);
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

Entity* delent_function(Entity** entity_table, String name)
{
  unsigned long value = entity_hash_function(name);
  int pos = value % ENTITY_TABLE_SIZE;
  Entity* entity = entity_table[pos];
  if (entity_table[pos] == NULL)
  {
    #ifdef deb
      printf("invalid entity.\n");
    #endif
    return 0;
  }
  entity_table[pos] = NULL;
  #ifdef deb
    printf("deallocated entity at %x\n", entity);
  #endif
  if (entity->rel_num != 0)
    return entity;
  return 0;
}
//deallocates an entity; return 1 if succes, else 0

void delete_unused_relation(Relation* relations_buffer[RELATIONS_BUFFER_SIZE], Relation* rel, int start_pos)
{
  //relations_table[hash][pos] = NULL;
  number_of_relations --;
  rel_buffer_fixup_delete(relations_buffer, start_pos);
  free(rel);
}

int delrel_function(Entity** entity_table, Relation* relations_buffer[RELATIONS_BUFFER_SIZE], String name_source, String name_dest, String rel_name)
{
  Relation* rel = get_relation(relations_buffer, rel_name);

  if (rel == 0)
  {
    #ifdef deb
      printf("relation does not exist.\n");
    #endif
    return 0;
  }

  Entity* src = get_entity(entity_table, name_source);
  if(src == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("one of the entities does not exist.\n");
    #endif
    return 0;
  }
  Entity* dest = get_entity(entity_table, name_dest);
  if(dest == 0) //check if entities and relation exist.
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
  i = sub_relations_binary_search(array, name_dest, 0, rel->sub_relation_number-1, src);
  if (i > -1)
  {
      sub_rel = array[i];
      free(sub_rel);
      rel->sub_relation_number --;
      src->rel_num --;
      dest->rel_num --;
      sub_rel_array_fixup_delete((void**)array, i, rel->sub_relation_number);
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

int check_entity_validity(Entity* entity, Entity** array, int del_num)
{
  while (del_num != 0)
    if (entity == array[--del_num])
      return 0;
  return 1;
}
//checks if the given entity hasn't been deleted.

char* fast_strcat(char* dest, char* src)
{
  while (*dest++ = *src++);
  return --dest;
}

int report_function(Entity** entity_table, Relation* relations_buffer[RELATIONS_BUFFER_SIZE], int update, Entity** del_array, int del_num)
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

  //get maximums and load entities in 2d array

  if (del_num > 0) {

  #ifdef deb
    printf("relations have been deleted.\n");
  #endif

  for (int h=0; h<number_of_relations; h++)
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
        prev_entity = prev_rel->destination;
        temp_counter = maximum = 0;
        for (k=0; k<rel_num; k++)
        {
          sub_rel = array[k];
          if (check_entity_validity(sub_rel->destination, del_array, del_num) == 1)
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
                  printf("%d: %s, %d;\n", i, prev_entity->name, temp_counter);
                #endif
                prev_entity = temp_entity;
                prev_rel = sub_rel;
                temp_counter = 1;
                #ifdef deb
                  printf("found a new entity, %s\n", temp_entity->name);
                #endif
            }
          }
          else
          {
            free(array[k]);
            relation->sub_relation_number --;
            rel_num --;
            sub_rel_array_fixup_delete((void**)array, k, rel_num);
            k--;
            #ifdef deb
              printf("deleted relation at position %d\n", k);
            #endif
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
    }
    else
    {
    #ifdef deb
      printf("relation %s is unused, deleting\n", relation->name);
    #endif
      delete_unused_relation(relations_buffer, relation, h);
      h --;
    }
  }

  } else {

  #ifdef deb
    printf("relations have NOT been deleted.\n");
  #endif

  for (int h=0; h<number_of_relations; h++)
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
          h --;
        }
    }

  }

  strcat(report_string, "\n");
  #ifdef deb
    printf("finished phase one, printing...\n");
  #endif

  }

  fputs_unlocked(report_string, stdout);
  return 1;
}

//debug functions---------------------------------------------------------------

#ifdef deb

void deb_print_entities(Entity** entity_table)
{
  for (int i=0; i<ENTITY_TABLE_SIZE; i++)
  {
    if (entity_table[i] != NULL)
      printf("(%d) %s, %lu\n", i, entity_table[i]->name, entity_table[i]->value);
  }
}

void deb_print_sub_relations(Relation* rel)
{
  int rel_num = rel->sub_relation_number;
  SubRelation** array = (SubRelation**) rel->sub_rel_array;
  for(int i=0; i<rel_num; i++)
  {
      if (array[i] != NULL)
      {
        Entity** src_buffer = array[i]->source_buffer;
        Entity* dest = array[i]->destination;
        int src_num = array[i]->src_num;
        for (int j=0; j<src_num; j++)
        {
          printf("%d (%d): %s --%s--> %s\n", i, j, src_buffer[j]->name, rel->name, dest->name);
        }
      }
      else
        printf("%d: NULLED\n", i);
  }
}

void deb_print_relations(Entity** entity_table, Relation** relations_buffer)
{
  for (int i=0; i<number_of_relations; i++)
  {
    if (relations_buffer[i]->sub_relation_number > 0)
      deb_print_sub_relations(relations_buffer[i]);
  }
}

#endif

//------------------------------------------------------------------------------
int main() //main program
{
  //main data structures
  static Entity* entity_table[ENTITY_TABLE_SIZE];
  Entity* deleted_entities_array[1024];
  int del_ent_num = 0;
  Relation* relations_buffer[RELATIONS_BUFFER_SIZE];
  //aux variables
  String argument0;
  String argument1;
  String argument2;
  LongString input_string;
  int report_needed = 1; // 0 = report has not changed, 1 = report has changed.
  #ifdef deb
    int command_counter = 0;
  #endif

  while (1)
  {
    fgets_unlocked(input_string, sizeof(LongString), stdin);
    char* p;

    #ifdef deb
      printf("\nCOMMAND %d: %s\nCOLLISIONS: %d\n\n", command_counter, input_string, collisions);
      command_counter ++;
    #endif
    switch (input_string[0]) {
    case 'a': //addent or addrel
    {
      if (input_string[3] == 'e') //addent
      {
        get_argument(input_string+8, argument0);
        handle_entity_creation(entity_table, argument0);
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      else //addrel
      {
        p = get_argument(input_string+8, argument0);
        p = get_argument(p, argument1);
        get_argument(p, argument2);
        if (handle_relation_creation(entity_table, relations_buffer,  argument0, argument1, argument2) != 0)
          report_needed = 1;
        #ifdef deb
          //deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      break;
    }
    case 'd': //delent or delrel
    {
      if (input_string[3] == 'e') //delent
      {
        get_argument(input_string+8, argument0);
        Entity* ent = delent_function(entity_table, argument0);
        if (ent != 0)
        {
          deleted_entities_array[del_ent_num] = ent;
          del_ent_num ++;
          report_needed = 1;
        }
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      else //delrel
      {
        p = get_argument(input_string+8, argument0);
        p = get_argument(p, argument1);
        get_argument(p, argument2);
        if (delrel_function(entity_table, relations_buffer, argument0, argument1, argument2) != 0)
          report_needed = 1;
        #ifdef deb
          deb_print_entities(entity_table);
          deb_print_relations(entity_table, relations_buffer);
        #endif
      }
      break;
    }
    case 'r': //report
    {
      if (report_function(entity_table, relations_buffer, report_needed, deleted_entities_array, del_ent_num) != 0)
      {
        del_ent_num = 0;
        report_needed = 0;
      }
      #ifdef deb
        deb_print_entities(entity_table);
        deb_print_relations(entity_table, relations_buffer);
      #endif
      break;
    }
    case 'e':
    {
      return 0;
    }

    }
  }
}
