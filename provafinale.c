#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUB_REL_TABLE_SIZE 499
#define ENTITY_TABLE_SIZE 91711
#define RELATIONS_TABLE_SIZE 353
#define ENT_ARRAY_SIZE 1024

/* add -Ddeb to gcc compiler options to compile in verbose debug mode */

//type definitions--------------------------------------------------------------

typedef char LongString[160];
typedef char String[50];

typedef struct {
  String name;
  int hash_value;
  void* ordered_relation; //points to the ordered relation.
  void* table_next; //Relation*
  void* table_prev; //Relation*
} Relation;

typedef struct {
  Relation* relation; //points to the relation in hash table
  int position; //position in the list
  void* next;  //OrdRel*
  void* prev;  //OrdRel*
} OrdRel; //for speed and redundancy

typedef struct {
  String name;

  int sub_relations_number;
  void* sub_relations[SUB_REL_TABLE_SIZE];  //in relations
  int first_sub_rel;
  int last_sub_rel;

  int entity_id; //add 1 when entity changes.
  int valid;     // = 1 when valid, else = 0.
  void* table_next; //Entity*

  void* array;  //for speedup
  int array_position;
} Entity;

typedef struct {
  Relation* relation;
  Entity* source;
  int source_id;
  void* table_next; //SubRelation*
} SubRelation;  //the destination is the entity that contains it.

typedef struct {
  Entity* entity;
  int occurrences;
  void* next;
} PopEntity; //struct used during report

Entity* entity_table[ENTITY_TABLE_SIZE];          // = table 0
Relation* relations_table[RELATIONS_TABLE_SIZE];  // = table 1

OrdRel* rel_list_head = NULL;
int number_of_relations = 0;

int number_of_entities = 0;
Entity* entities_array_first;

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

void* global_hash_table_linear_search(String name, int table_pos, const int table)
{
  switch(table)
  {
    case 0: //entity table
    {
      Entity* entity = entity_table[table_pos];
      while (entity != NULL)
      {
        if (strcmp(name, entity->name) == 0)
        {
          #ifdef deb
            printf("linear search done, found entity\n");
          #endif
          return entity;
        }
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
        {
          #ifdef deb
            printf("linear search done, found relation\n");
          #endif
          return rel;
        }
        rel = (Relation*) rel->table_next;
      }
      break;
    }
  }
  #ifdef deb
    printf("linear search done, no result\n");
  #endif
  return 0;
}
//returns 0 if nothing is found, else it returns the pointer
//of the wanted thing

Entity* get_entity(String name)
{
  int pos = hash_function(name, ENTITY_TABLE_SIZE);
  Entity* entity = (Entity*) global_hash_table_linear_search(name, pos, 0);
  return entity;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

Entity* get_entity_prev(String name, int pos)
{
  Entity* entity = (Entity*) entity_table[pos];
  Entity* prev_entity = entity;
  while(entity != NULL)
  {
    entity = (Entity*) entity->table_next;
    if (strcmp(name, entity->name) == 0)
    {
      #ifdef deb
        printf("found entity to delete in linear search.\n");
      #endif
      return prev_entity;
    }
    prev_entity = entity;
  }
  #ifdef deb
    printf("no entry found to delete.\n");
  #endif
  return 0;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return entity.

Relation* get_relation(String name)
{
  int pos = hash_function(name, RELATIONS_TABLE_SIZE);
  Relation* entity = (Relation*) global_hash_table_linear_search(name, pos, 1);
  return entity;
}
//check the hash table and see if the entity already exists,
//return 0 if it does not, else return relation.

Entity* create_entity(String name)
{
  //create the entity in the hash table
  Entity* self;
  self = (Entity*) malloc(sizeof(Entity));
  strcpy(self->name, name);
  self->table_next = NULL;
  self->entity_id = 0;
  self->valid = 1;
  self->first_sub_rel = SUB_REL_TABLE_SIZE;
  self->last_sub_rel = 0;
  number_of_entities ++;
  //add entity to entity array

  return self;
}
//crea entità, assegna il nome e ritorna il puntatore.

void reallocate_entity(Entity* self, String name)
{
  strcpy(self->name, name);
  self->valid = 1;
}

void deallocate_entity(Entity* self)
{
  //strcpy(self->name, "");
  self->entity_id ++;
  self->valid = 0;
  self->first_sub_rel = SUB_REL_TABLE_SIZE;
  self->last_sub_rel = 0;
}

void ordered_relation_list_fixup(OrdRel* start_rel, int pos)
{
  OrdRel* rel = start_rel;
  while (rel != NULL)
  {
    rel->position = pos;
    pos ++;
    rel = (OrdRel*) rel->next;
  }
}
//fixes the order of the ordered list from the indicated starting position

Relation* create_relation(String name, int hash)
{
  //create the relation to put in hash table
  Relation* self;
  self = (Relation*) malloc(sizeof(Relation));
  strcpy(self->name, name);
  self->hash_value = hash;
  self->table_next = NULL;
  self->table_prev = NULL;
  //create ordered list entry
  OrdRel* ord = (OrdRel*) malloc(sizeof(OrdRel));
  ord->relation = self;
  self->ordered_relation = ord;
  OrdRel* pointer = rel_list_head;
  OrdRel* prev_pointer = NULL;
  //put entry in ordered list
  while (pointer != NULL)
  {
    if (strcmp(self->name, (pointer->relation)->name) < 0)
    {
      if (prev_pointer == NULL)
      {
        ord->next = rel_list_head;
        ord->prev = NULL;
        rel_list_head = ord;
        ordered_relation_list_fixup(ord, 0);
        number_of_relations ++;
        return self;
      }
      prev_pointer->next = ord;
      ord->prev = prev_pointer;
      ord->next = pointer;
      ordered_relation_list_fixup(ord, prev_pointer->position+1);
      number_of_relations ++;
      return self;
    }
    prev_pointer = pointer;
    pointer = (OrdRel*) pointer->next;
  }

  if (prev_pointer == NULL)
  {
    rel_list_head = ord;
    ord->prev = NULL;
    ord->position = 0;
  }
  else
  {
    prev_pointer->next = ord;
    ord->prev = prev_pointer;
    ord->position = number_of_relations;
  }
  ord->next = NULL;
  number_of_relations ++;
  return self;
}
//Create relation in hash table, create relation in ordered list and
//return the pointer to the relation in hash table.

SubRelation* create_sub_relation(Entity* src, Relation* rel)
{
  SubRelation* self = (SubRelation*) malloc(sizeof(SubRelation));
  self->source = src;
  self->relation = rel;
  self->source_id = src->entity_id;
  self->table_next = NULL;
  return self;
}

void set_sub_relation_first_and_last(Entity* entity, int pos)
{
  if (entity->first_sub_rel > pos)
    entity->first_sub_rel = pos;
  if (entity->last_sub_rel < pos)
    entity->last_sub_rel = pos;
}

int relation_add_entities(Relation* rel, Entity* src, Entity* dest)
{
  int pos = hash_function(src->name, SUB_REL_TABLE_SIZE);
  SubRelation** start = (SubRelation**) (dest->sub_relations+pos);
  if (*start == NULL)
  {
    *start = create_sub_relation(src, rel);
    set_sub_relation_first_and_last(dest, pos);
    #ifdef deb
      printf("Added entries to relation, first place\n");
    #endif
  }
  else
  {
    SubRelation* sub_rel = *start;
    SubRelation* prev_rel;
    while(sub_rel != NULL)
    {
      prev_rel = sub_rel;
      if (src == (sub_rel)->source && rel == (sub_rel)->relation)
      {
        #ifdef deb
          printf("Relation already exists.\n");
        #endif
        return 1;
      }
      sub_rel = (SubRelation*) sub_rel->table_next;
    }
    sub_rel = create_sub_relation(src, rel);
    prev_rel->table_next = sub_rel;
    set_sub_relation_first_and_last(dest, pos);
    #ifdef deb
      printf("Added entries to relation\n");
    #endif
  }
  return 0;
}
//adds entities to the new/existing relation;
//return 0 if everything went okay, else 1.

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
    Entity* prev_entity;
    while (entity != NULL)
    {
      prev_entity = entity;
      if (entity->valid == 0)
      {
        reallocate_entity(entity, name);
        #ifdef deb
          printf("Reallocated entity\n");
        #endif
        return entity;
      }
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
    prev_entity->table_next = entity;
    return entity;
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle entity creation, return the created entity if success, else 0

Relation* handle_relation_creation(String name1, String name2, String name)
{
  int pos = hash_function(name, RELATIONS_TABLE_SIZE);
  Entity* e1 = get_entity(name1);
  Entity* e2 = get_entity(name2);

  if (e1 == 0 || e2 == 0) //check if entities exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  if (relations_table[pos] == NULL)
  {
    relations_table[pos] = create_relation(name, pos);
    #ifdef deb
      printf("Created new relation in empty table slot\n");
    #endif
    relation_add_entities(relations_table[pos], e1, e2);
    return relations_table[pos];
  }
  else
  {
    Relation* rel = relations_table[pos];
    Relation* prev_rel;
    while (rel != NULL)
    {
      prev_rel = rel;
      if (strcmp(name, rel->name) == 0)
      {
        relation_add_entities(rel, e1, e2);
        return rel;
      }
      rel = (Relation*) rel->table_next;
    }
    rel = create_relation(name, pos);
    prev_rel->table_next = rel;
    rel->table_prev = prev_rel;
    #ifdef deb
      printf("Created new relation\n");
    #endif
    relation_add_entities(rel, e1, e2);
    return rel;
  }
  #ifdef deb
    printf("Error\n");
  #endif
  return 0;
}
//handle relation creation, return 0 if nothing was created, the relation
//created or modified otherwise.

SubRelation* delete_sub_relation(SubRelation* sub_rel)
{
  SubRelation* next = (SubRelation*) sub_rel->table_next;
  free(sub_rel);
  return next;
}
//deletes the specified sub relation, return the next pointer.
//if there is no next pointer, return NULL.

void delete_relation_stack(Entity* entity)
{
  SubRelation* next;
  SubRelation** start;
  for(int i=0; i<SUB_REL_TABLE_SIZE; i++)
  {
    start = (SubRelation**) (entity->sub_relations+i);
    while (*start != NULL)
    {
      next = (SubRelation*) (*start)->table_next;
      free(*start);
      *start = next;
      #ifdef deb
        printf("deleted relation\n");
      #endif
    }
  }
}
//delete all the relations towards the deleted entity.

int delent_function(String name)
{
  Entity* entity = get_entity(name);
  if (entity == 0)
  {
    #ifdef deb
      printf("invalid entity.\n");
    #endif
    return 1;
  }
  delete_relation_stack(entity);
  deallocate_entity(entity);
  #ifdef deb
    printf("deallocated entity at %x\n", entity);
  #endif
  return 0;
}
//deallocates an entity; return 0 if succes, else 1

int delrel_function(String name_source, String name_dest, String rel_name)
{
  Relation* rel = get_relation(rel_name);
  Entity* e1 = get_entity(name_source);
  Entity* e_dest = get_entity(name_dest);

  if (e1 == 0 || e_dest == 0 || rel == 0) //check if entities and relation exist.
  {
    #ifdef deb
      printf("one of the entities does not exist\n");
    #endif
    return 0;
  }

  int sub_pos = hash_function(name_source, SUB_REL_TABLE_SIZE);
  SubRelation** start = (SubRelation**) (e_dest->sub_relations+sub_pos);
  if (*start != NULL)
  {
    SubRelation* sub_rel = *start;
    SubRelation* prev_rel = NULL;
    while(sub_rel != NULL)
    {
      if (sub_rel->source == e1 && sub_rel->relation == rel)
      {
        if (prev_rel == NULL)
        {
          *start = delete_sub_relation(sub_rel);
          return sub_pos;
        }
        else
        {
          prev_rel->table_next = delete_sub_relation(sub_rel);
          return sub_pos;
        }
      }
      prev_rel = sub_rel;
      sub_rel = (SubRelation*) sub_rel->table_next;
    }
  }
  #ifdef deb
    printf("no relation found.\n");
  #endif
  return sub_pos;
}
//return the postion of the relation in the sub array for the specific relation

SubRelation* delete_obsolete_relation(int i, SubRelation* prev_rel, Entity* entity, SubRelation* sub_rel)
{
  if (prev_rel == NULL)
  {
    *(entity->sub_relations+i) = delete_sub_relation(sub_rel);
    sub_rel = (SubRelation*) *(entity->sub_relations+i);
  }
  else
  {
    prev_rel->table_next = delete_sub_relation(sub_rel);
    sub_rel = (SubRelation*) prev_rel->table_next;
  }
  return sub_rel;
}
//deletes an obsolete sub relation, returns the next relation (if there is no
//next it returns NULL).

OrdRel* delete_unused_relation(Relation* rel)
{
  //setup pointers
  Relation* rel_prev = (Relation*) rel->table_prev;
  Relation* rel_next = (Relation*) rel->table_next;
  OrdRel* ord_rel = (OrdRel*) rel->ordered_relation;
  OrdRel* ord_rel_prev = (OrdRel*) ord_rel->prev;
  OrdRel* ord_rel_next = (OrdRel*) ord_rel->next;
  //delete the relation
  if (rel_prev == NULL)
    relations_table[rel->hash_value] = rel_next;
  else
    rel_prev->table_next = rel_next;
  free(rel);
  //delete the ordered relation
  if (ord_rel_prev == NULL)
    rel_list_head = ord_rel_next;
  else
    ord_rel_prev->next = ord_rel_next;
  free(ord_rel);
  //decrease counter
  number_of_relations --;
  return ord_rel_next;
}
//if it detects that a relation has 0 instances, it deletes it;
//called from report_function.

PopEntity* create_pop_entity(Entity* entity)
{
  PopEntity* pop = (PopEntity*) malloc(sizeof(PopEntity));
  pop->entity = entity;
  pop->occurrences = 1;
  pop->next = NULL;
  #ifdef deb
    printf("created popular entity %s\n", entity->name);
  #endif
  return pop;
}

int report_function()
{
  //if there are no relations, do nothing
  if (rel_list_head == NULL)
  {
    printf("none\n");
    return 0;
  }

  Entity* entity;
  Entity* src;
  Relation* relation;
  SubRelation* prev_rel;
  SubRelation* sub_rel;

  int i, j, position;
  int max_value_temp[number_of_relations];
  PopEntity* popular_entities_temp[number_of_relations];

  for (i=0; i<number_of_relations; i++)
    popular_entities_temp[i] = NULL;

  //set all the maximums to 0
  for (i=0; i<number_of_relations; i++)
    max_value_temp[i] = 0;
    #ifdef deb
      printf("starting phase one.\n");
    #endif
  for (j=0; j<ENTITY_TABLE_SIZE; j++)
  {
    //load the entities popularity for each relation in the temp array
    if (entity_table[j] != NULL)
    {
      entity = entity_table[j];
      while (entity != NULL)
      {
        #ifdef deb
          printf("found an entity, %s\n", entity->name);
        #endif
        if (entity->valid == 1)
        {
          #ifdef deb
            printf("the entity is valid\n");
          #endif
          int last_relation = entity->last_sub_rel + 1;
          for (int i=entity->first_sub_rel; i<last_relation; i++)
          {
            sub_rel = (SubRelation*) *(entity->sub_relations+i);
            prev_rel = NULL;
            while (sub_rel != NULL)
            {
              #ifdef deb
                printf("found a relation\n");
              #endif
              src = sub_rel->source;
              relation = sub_rel->relation;
              if (src->valid == 1)
              {
                if (sub_rel->source_id == src->entity_id)
                {
                  position = ((OrdRel*)(relation->ordered_relation))->position;
                  #ifdef deb
                    printf("the relation is valid, pos: %d\n", position);
                  #endif
                  if (popular_entities_temp[position] == NULL)
                  {
                    #ifdef deb
                      printf("first position is free\n");
                    #endif
                    popular_entities_temp[position] = create_pop_entity(entity);
                    max_value_temp[position] = 1;
                    #ifdef deb
                      printf("max[%d] = %d\n", position, max_value_temp[position]);
                      printf("entity added: %s\n", (popular_entities_temp[position]->entity)->name);
                    #endif
                  }
                  else
                  {
                    #ifdef deb
                      printf("first position is not free\n");
                    #endif
                    PopEntity* pe = popular_entities_temp[position];
                    PopEntity* pe_prev = NULL;
                    PopEntity* destination;
                    while(pe != NULL)
                    {
                      #ifdef deb
                        printf("%s, \n", (pe->entity)->name);
                      #endif
                      if (pe->entity == entity)
                      {
                        #ifdef deb
                          printf("same dest found, increasing max.\n");
                        #endif
                        pe->occurrences ++;
                        if (max_value_temp[position] < pe->occurrences)
                        {
                          max_value_temp[position] = pe->occurrences;
                          #ifdef deb
                            printf("entity found: %s\n", (pe->entity)->name);
                            printf("max[%d] = %d\n", position, max_value_temp[position]);
                          #endif
                        }
                        break;
                      }
                      else
                      {
                        #ifdef deb
                          printf("the entity has to be put in the table\n");
                        #endif
                        if (strcmp(entity->name, ((Entity*)(pe->entity))->name) < 0)
                        {
                          #ifdef deb
                            printf("the entity is inferior\n");
                          #endif
                          destination = pe_prev;
                          pe = NULL;
                        }
                        else
                        {
                          #ifdef deb
                            printf("the entity is superior\n");
                          #endif
                          destination = pe;
                          pe_prev = pe;
                          pe = (PopEntity*) pe->next;
                        }
                      }
                    }
                    if (pe == NULL)
                    {
                      //add a popular entity.
                      if (destination == NULL)
                      {
                        pe = popular_entities_temp[position];
                        popular_entities_temp[position] = create_pop_entity(entity);
                        popular_entities_temp[position]->next = pe;
                        #ifdef deb
                          printf("putting entity first\n");
                          char* start = (popular_entities_temp[position]->entity)->name;
                          char* next = (((PopEntity*)popular_entities_temp[position]->next)->entity)->name;
                          printf("start: %s, next: %s\n", start, next);
                        #endif
                      }
                      else
                      {
                        pe = (PopEntity*) destination->next;
                        destination->next = create_pop_entity(entity);
                        ((PopEntity*) destination->next)->next = pe;
                        #ifdef deb
                          printf("putting entity in the middle\n");
                          char* prev = (destination->entity)->name;
                          char* current = (((PopEntity*)destination->next)->entity)->name;
                          char* next = (pe->entity)->name;
                          printf("start: %s, current: %s, next: %s\n", prev, current, next);
                        #endif
                      }
                    }
                  }
                  prev_rel = sub_rel;
                  sub_rel = (SubRelation*) sub_rel->table_next;
                }
                else
                  sub_rel = delete_obsolete_relation(i, prev_rel, entity, sub_rel);
              }
              else
                sub_rel = delete_obsolete_relation(i, prev_rel, entity, sub_rel);
            }
          }
        }
        entity = (Entity*) entity->table_next;
      }
    }
  }
  #ifdef deb
    for (i=0; i<number_of_relations; i++)
    {
      printf("%d: ", i);
      if (popular_entities_temp[i] != NULL)
      {
        PopEntity* tmp = popular_entities_temp[i];
        while (tmp !=  NULL)
        {
          printf("%s, %d; ", (tmp->entity)->name, tmp->occurrences);
          tmp = tmp->next;
        }
      }
      printf("\n");
    }
    printf("phase one finished, starting phase two.\n");
  #endif
  OrdRel* ord_rel = rel_list_head;

  int max_value[number_of_relations];
  PopEntity* popular_entities[number_of_relations];
  String relation_name[number_of_relations];
  j = i = 0;

  while(ord_rel != NULL)
  {
    #ifdef deb
      printf("found relation\n");
    #endif
    //if the max value for a relation is zero, it gets eliminated.
    if (max_value_temp[i] == 0)
    {
      #ifdef deb
        printf("deleting unused relation\n");
      #endif
      ord_rel = delete_unused_relation(ord_rel->relation);
    }
    else
    {
      max_value[j] = max_value_temp[i];
      PopEntity* pop_ent = popular_entities_temp[i];
      PopEntity* prev_ent = NULL;
      while (pop_ent != NULL)
      {
        if (pop_ent->occurrences < max_value[j])
        {
          PopEntity* temp;
          if (prev_ent == NULL)
          {
            temp = (PopEntity*) popular_entities_temp[i]->next;
            free(popular_entities_temp[i]);
            popular_entities_temp[i] = temp;
          }
          else
          {
            temp = (PopEntity*) pop_ent->next;
            free(pop_ent);
            prev_ent->next = temp;
          }
          pop_ent = temp;
        }
        else
        {
          prev_ent = pop_ent;
          pop_ent = (PopEntity*) pop_ent -> next;
        }
      }
      popular_entities[j] = popular_entities_temp[i];
      strcpy(relation_name[j], (ord_rel->relation)->name);
      ord_rel = (OrdRel*) ord_rel->next;
      j ++;
    }
    i ++;
  }
    #ifdef deb
    for (i=0; i<number_of_relations; i++)
    {
      printf("%d: ", i);
      if (popular_entities[i] != NULL)
      {
        PopEntity* tmp = popular_entities[i];
        while (tmp !=  NULL)
        {
          printf("%s, %d; ", (tmp->entity)->name, tmp->occurrences);
          tmp = tmp->next;
        }
      }
      printf("\n");
    }
    printf("phase two finished, starting phase three.\n");
  #endif
  ordered_relation_list_fixup(rel_list_head, 0);
  PopEntity* pop_entity;
  for(i=0; i<number_of_relations; i++)
  {
    //print required stuff
    printf("%s ", relation_name[i]);
    pop_entity = popular_entities[i];
    PopEntity* pe;
    while (pop_entity != NULL)
    {
      printf("%s ", (pop_entity->entity)->name);
      pe = (PopEntity*) pop_entity->next;
      free(pop_entity);
      pop_entity = pe;
    }
    printf("%d; ", max_value[i]);
  }
  printf("\n");
  return 0;
}

//debug functions---------------------------------------------------------------

#ifdef deb

void deb_print_entities()
{
  Entity* entity;
  for (int i=0; i<ENTITY_TABLE_SIZE; i++)
  {
    if (entity_table[i] != NULL)
    {
        entity = entity_table[i];
        printf("%d: ", i);
        while (entity != NULL)
        {
          if (entity->valid == 1)
            printf("%s [id = %d], ", entity->name, entity->entity_id);
          entity = (Entity*) entity->table_next;
        }
        printf("\n");
    }
  }
}

void deb_print_sub_relations(Entity* entity)
{
  Entity* src;
  Relation* relation;
  SubRelation* prev_rel;
  for (int i=0; i<SUB_REL_TABLE_SIZE; i++)
  {
    SubRelation* sub_rel = (SubRelation*) *(entity->sub_relations+i);
    prev_rel = NULL;
    while (sub_rel != NULL)
    {
      src = sub_rel->source;
      relation = sub_rel->relation;
      if (src->valid == 1)
      {
        if (sub_rel->source_id == src->entity_id)
        {
          printf("%s --%s-> %s\n", src->name, relation->name, entity->name);
          prev_rel = sub_rel;
          sub_rel = (SubRelation*) sub_rel->table_next;
        }
        else
          sub_rel = delete_obsolete_relation(i, prev_rel, entity, sub_rel);
      }
      else
        sub_rel = delete_obsolete_relation(i, prev_rel, entity, sub_rel);
    }
  }
}

void deb_print_relations()
{
  Entity* dest;
  for (int i=0; i<ENTITY_TABLE_SIZE; i++)
  {
    if (entity_table[i] != NULL)
    {
      dest = entity_table[i];
      while (dest != NULL)
      {
        deb_print_sub_relations(dest);
        dest = (Entity*) dest->table_next;
      }
    }
  }
}

void deb_print_ordered_relations()
{
  OrdRel* pointer = rel_list_head;
  while(pointer != NULL)
  {
    printf("%d: %s\n", pointer->position, (pointer->relation)->name);
    pointer = (OrdRel*) pointer->next;
  }
}

void deb_print_all()
{
  deb_print_entities();
  deb_print_relations();
  deb_print_ordered_relations();
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
        #ifdef deb
          deb_print_relations();
        #endif
        break;
      }
      case 2: //delent
      {
        get_argument(input_string, argument0, 7);
        delent_function(argument0);
        #ifdef deb
          deb_print_entities();
        #endif
        break;
      }
      case 3: //delrel
      {
        opcode = get_argument(input_string, argument0, 7);
        opcode = get_argument(input_string, argument1, opcode);
        get_argument(input_string, argument2, opcode);
        delrel_function(argument0, argument1, argument2);
        #ifdef deb
          deb_print_relations();
        #endif
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
