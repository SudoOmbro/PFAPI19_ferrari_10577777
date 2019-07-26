#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITY_TABLE_SIZE  49783
#define SUB_RELATIONS_ARRAY_SIZE 80000 //65536
#define RELATIONS_BUFFER_SIZE 4096 //32768
#define COLLISION_BUFFER_SIZE 512

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char SuperLongString[8192];
typedef char LongString[160];
typedef char String[50];

typedef struct {
  String name;
  int sub_relation_number;
  void* sub_rel_array[SUB_RELATIONS_ARRAY_SIZE];
} Relation;

typedef struct {
  String name;
  int entity_id; //add 1 when entity changes.
  int valid;     // = 1 when valid, else = 0.
  unsigned long value;
} Entity;

typedef struct {
  Entity* source;
  Entity* destination;
  int source_id;
  int destination_id;
  unsigned long source_value;
  unsigned long destination_value;
} SubRelation;  //the destination is the entity that contains it.

typedef struct {
  Entity* entity;
  int occurrences;
} Pop; //struct used during report

int number_of_entities = 0;
Entity* entity_table[ENTITY_TABLE_SIZE][COLLISION_BUFFER_SIZE];

Relation* relations_buffer[RELATIONS_BUFFER_SIZE];
int number_of_relations = 0;

//------------------------------------------------------------------------------
//functions definitions---------------------------------------------------------

int entity_hash_function(char* text)
{
  unsigned long value = 0;
  for (int i=0; text[i] != '\0'; i++)
  {
    value = 131*value + text[i];
  }
  #ifdef deb
    printf("entity hash value: %d\n", (value) % ENTITY_TABLE_SIZE);
  #endif
  return (value) % ENTITY_TABLE_SIZE;
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
  int i = 0;
  while (entity_table[table_pos][i] != NULL)
  {
    if (entity_table[table_pos][i]->valid == 1)
    {
      if (strcmp(name, entity_table[table_pos][i]->name) == 0)
      {
        #ifdef deb
          printf("linear search done, found entity\n");
        #endif
        return entity_table[table_pos][i];
      }
    }
    i ++;
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
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
  #ifdef deb
    printf("getting entity %s.\n", name);
  #endif
  int pos = entity_hash_function(name);
  return entity_hash_table_linear_search(name, pos);
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

int relations_binary_search(String name, int l, int r)
{
  if (r >= l)
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
            return relations_binary_search(name, l, mid - 1);
        return relations_binary_search(name, mid + 1, r);
  }
  #ifdef deb
    printf("no match found for %s.\n", name);
  #endif
  return -1;
}

int sub_relations_binary_search(SubRelation** array, String name, int l, int r, unsigned long src_val)
{
  if (r >= l)
  {
        #ifdef deb
          printf("sub rel binary search %d, %d\n", l, r);
        #endif
        int mid = l + (r - l) / 2;
        int out_val = strcmp(name, (array[mid]->destination)->name);
        if (out_val == 0)
        {
            #ifdef deb
              printf("checking entity %s.\n", (array[mid]->source)->name);
            #endif
            unsigned long val = array[mid]->source_value;
            if (val == src_val)
            {
              return mid;
            }
            if (val > src_val)
                return sub_relations_binary_search(array, name, l, mid - 1, src_val);
            return sub_relations_binary_search(array, name, mid + 1, r, src_val);
        }
        #ifdef deb
          printf("nope.\n");
        #endif
        if (out_val < 0)
            return sub_relations_binary_search(array, name, l, mid - 1, src_val);
        return sub_relations_binary_search(array, name, mid + 1, r, src_val);
  }
  #ifdef deb
    printf("no match found for %s.\n", name);
  #endif
  return -1;
}

int sub_relations_binary_search_creation(SubRelation** array, String dest_name, int l, int r, unsigned long src_value)
{
  if (r >= l)
  {
        #ifdef deb
          printf("binary search %d, %d\n", l, r);
        #endif
        int mid = l + (r - l) / 2;
        int last_valid;
        int ext_val = strcmp(dest_name, (array[mid]->destination)->name);
        unsigned long val;
        if (ext_val == 0)
        {
            int sign;
            unsigned long value = array[mid]->destination_value;
            last_valid = mid;
            val = array[mid]->source_value;
            if (val < src_value)
              sign = 1;
            else
              sign = -1;

            #ifdef deb
              printf("found destination %s.\n", dest_name);
            #endif
            while (value == array[mid]->destination_value)
            {
              #ifdef deb
                printf("checking %s\n", (array[mid]->source)->name);
                printf("src: %lu, pnt: %lu\n", src_value, val);
              #endif
              if (src_value == val)
              {
                #ifdef deb
                  printf("relation already exists.\n");
                #endif
                return -1;
              }
              if (val < src_value)
              {
                if (sign == -1)
                {
                  #ifdef deb
                    printf("returning BG: %d\n", last_valid);
                  #endif
                  return last_valid;
                }
                #ifdef deb
                  printf("%lu is bigger than %lu\n", src_value, val);
                #endif
                last_valid = mid;
                mid ++;
                sign = 1;
              }
              else
              {
                if (sign == 1)
                {
                  #ifdef deb
                    printf("returning LW: %d\n", last_valid+1);
                  #endif
                  return last_valid+1;
                }
                #ifdef deb
                  printf("%ld is smaller than %ld\n", src_value, val);
                #endif
                last_valid = mid;
                mid --;
                sign = -1;
              }

              #ifdef deb
                printf("mid: %d\n", mid);
              #endif
              if (mid < 0)
                return 0;
              if (array[mid] == NULL)
                return mid;

              val = array[mid]->source_value;
            }
            if (sign == 1)
              return mid;
            return last_valid;
        }
        if (ext_val < 0)
        {
            #ifdef deb
              printf("int %s < %s\n", dest_name, (array[mid]->destination)->name);
            #endif
            return sub_relations_binary_search_creation(array, dest_name, l, mid - 1, src_value);
        }
        #ifdef deb
          printf("int %s > %s\n", dest_name, (array[mid]->destination)->name);
        #endif
        return sub_relations_binary_search_creation(array, dest_name, mid + 1, r, src_value);
  }
  #ifdef deb
    printf("r(%d) < l(%d)\n", r, l);
  #endif
  return l;
}

Relation* get_relation(String name)
{
  #ifdef deb
    printf("getting relation %s.\n", name);
  #endif

  if (number_of_relations == 0)
    return 0;

  int val = relations_binary_search(name, 0, number_of_relations-1);
  if (val >= 0)
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
  self->entity_id = 0;
  self->valid = 1;
  self->value = value;
  number_of_entities ++;
  return self;
}
//crea entità, assegna il nome e ritorna il puntatore.

void reallocate_entity(Entity* self, String name, unsigned long value)
{
  strcpy(self->name, name);
  self->valid = 1;
  self->value = value;
  number_of_entities ++;
}

void deallocate_entity(Entity* self)
{
  self->entity_id ++;
  self->valid = 0;
  //strcpy(self->name, "");
  //self->value = 0;
  number_of_entities --;
}

void sub_rel_array_fixup(SubRelation** array, int start_pos, const int max, SubRelation* replacer)
{
  SubRelation* temp;
  int i;
  for (i=start_pos; i<max; i++)
  {
    temp = array[i];
    array[i] = replacer;
    replacer = temp;
  }
  array[i] = NULL;
}
//given the pointer to the array to restore, the starting position, the
//end postion and the entity that replaces, this function restores
//the order in the array.

void rel_buffer_fixup(int start_pos, Relation* replacer)
{
  Relation* temp;
  for (int i=start_pos; i<number_of_relations; i++)
  {
    temp = relations_buffer[i];
    relations_buffer[i] = replacer;
    replacer = temp;
  }
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

void sub_rel_array_fixup_delete_optimized(SubRelation** array, int end)
{
  #ifdef deb
    printf("array fixup optimized, end: %d\n", end);
  #endif

  if (end > 0)
  {
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
}
//returns the number of deleted sub_releations.

void rel_buffer_fixup_delete(int start_pos)
{
  int i;
  for (i=start_pos; i<number_of_relations; i++)
    relations_buffer[i] = relations_buffer[i+1];
  relations_buffer[i] = NULL;
}

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
  self->source = src;
  self->destination = dest;
  self->source_id = src->entity_id;
  self->destination_id = dest->entity_id;
  self->source_value = src->value;
  self->destination_value = dest->value;
  return self;
}
//creates a sub relation and returns the pointer to that.

int relation_add_entities(Relation* rel, Entity* src, Entity* dest)
{
  //setup
  int rel_num = rel->sub_relation_number;
  SubRelation* sub_rel;
  SubRelation** array =  (SubRelation**) rel->sub_rel_array;
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
    int mid_val = sub_relations_binary_search_creation(array, dest->name, 0, rel_num-1, src->value);
    if (mid_val > -1)
    {
      #ifdef deb
        printf("found a spot\n");
      #endif
      sub_rel = create_sub_relation(src, dest);
      rel->sub_relation_number ++;
      sub_rel_array_fixup(array, mid_val, rel_num+1, sub_rel);
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

int handle_entity_creation(String name)
{
  //do the hash functio but also save the value of the entity
  unsigned long value = 0;
  for (int i=0; name[i] != '\0'; i++)
  {
    value = 131*value + name[i];
  }
  #ifdef deb
    printf("entity value: %d\n", value);
    printf("entity hash value: %d\n", (value) % ENTITY_TABLE_SIZE);
  #endif
  int pos = (value) % ENTITY_TABLE_SIZE;

  //add entity into hash table.
  if (entity_table[pos][0] == NULL)
  {
    entity_table[pos][0] = create_entity(name, value);
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
        entity_table[pos][i] = create_entity(name, value);
        return 1;
      }
      if (entity_table[pos][i]->valid == 0)
      {
        reallocate_entity(entity_table[pos][i], name, value);
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
  Entity* e1 = get_entity(name1);
  Entity* e2 = get_entity(name2);

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

  int i, val;
  if (number_of_relations == 0)
  {
    relations_buffer[0] = create_relation(name);
    number_of_relations ++;
    return relation_add_entities(relations_buffer[0], e1, e2);
  }
  for (i=0; i<number_of_relations; i++)
  {
    val = strcmp(name, relations_buffer[i]->name);
    if (val == 0)
    {
      return relation_add_entities(relations_buffer[i], e1, e2);
    }
    if (val < 0)
    {
      #ifdef deb
        printf("relation is smaller\n");
      #endif
      number_of_relations ++;
      Relation* self = create_relation(name);
      rel_buffer_fixup(i, self);
      return relation_add_entities(self, e1, e2);
    }
  }
  #ifdef deb
    printf("relation is bigger\n");
  #endif
  relations_buffer[i] = create_relation(name);
  number_of_relations ++;
  return relation_add_entities(relations_buffer[i], e1, e2);
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

  if (rel == 0)
  {
    #ifdef deb
      printf("relation does not exist.\n");
    #endif
    return 0;
  }

  Entity* src = get_entity(name_source);
  Entity* dest = get_entity(name_dest);
  SubRelation* sub_rel;
  int i;
  SubRelation** array;

  if(src == 0 || dest == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("one of the entities does not exist.\n");
    #endif
    return 0;
  }

  //delete sub relation from relation list in relation
  #ifdef deb
    printf("relation may exist.\n");
  #endif
  array = (SubRelation**) rel->sub_rel_array;
  i = sub_relations_binary_search(array, name_dest, 0, rel->sub_relation_number-1, src->value);
  if (i > -1)
  {
      sub_rel = array[i];
      free(sub_rel);
      rel->sub_relation_number --;
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

void delete_unused_relation(Relation* rel, int start_pos)
{
  //relations_table[hash][pos] = NULL;
  number_of_relations --;
  rel_buffer_fixup_delete(start_pos);
  free(rel);
}

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

    SuperLongString entities_string;

    //aux variables
    Entity* prev_entity;
    Entity* temp_entity;
    Relation* relation;
    SubRelation* sub_rel;
    SubRelation** array;
    char tmp_string[20];
    int i = 0, k, temp_counter = 0;
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
        temp_counter = maximum = 0;
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
                if (temp_counter > maximum)
                {
                  maximum = temp_counter;
                  strcpy(entities_string, prev_entity->name);
                  strcat(entities_string, " ");
                }
                else if (temp_counter == maximum)
                {
                  strcat(entities_string, prev_entity->name);
                  strcat(entities_string, " ");
                }
                #ifdef deb
                  printf("%d: %s, %d;\n", i, prev_entity->name, temp_counter);
                #endif
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

        if (temp_counter > maximum)
        {
          maximum = temp_counter;
          strcpy(entities_string, prev_entity->name);
          strcat(entities_string, " ");
        }
        else if (temp_counter != 0 && temp_counter == maximum)
        {
          strcat(entities_string, prev_entity->name);
          strcat(entities_string, " ");
        }

        if (maximum > 0)
        {
          strcat(report_string, relation->name);
          strcat(report_string, " ");
          strcat(report_string, entities_string);
          sprintf(tmp_string, "%d; ", maximum);
          strcat(report_string, tmp_string);
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
          delete_unused_relation(relation, h);
          h --;
        }
    }
    else
    {
      #ifdef deb
        printf("relation %s is unused, deleting\n", relation->name);
      #endif
      delete_unused_relation(relation, h);
      h --;
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
  LongString input_string;
  LongString prev_command = "";
  int report_needed = 1; // 0 = report has not changed, 1 = report has changed.
  #ifdef deb
    int command_counter = 0;
  #endif

  int opcode, prev_opcode = 5;
  while (1)
  {
    fgets(input_string, 160, stdin);
    opcode = generate_opcode(input_string);

    if (opcode != 4)
    {
      if (prev_opcode == opcode)
      {
        if (strcmp(prev_command, input_string) == 0)
          opcode = 6;
        else
        {
          prev_opcode = opcode;
          strcpy(prev_command, input_string);
        }
      }
      else
      {
        prev_opcode = opcode;
        strcpy(prev_command, input_string);
      }
    }

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
          deb_print_entities();
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
          deb_print_entities();
          deb_print_relations();
        #endif
        break;
      }
      case 4: //report
      {
        if (report_function(report_needed) != 0)
          report_needed = 0;
        #ifdef deb
          deb_print_entities();
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
