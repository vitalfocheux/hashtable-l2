#include "hashtable.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*
 * value
 */

enum value_kind value_get_kind(const struct value *self) {
  return self->kind;
}

bool value_is_nil(const struct value *self) {
  return value_get_kind(self) == VALUE_NIL;
}

bool value_is_boolean(const struct value *self) {
  return value_get_kind(self) == VALUE_BOOLEAN;
}

bool value_is_integer(const struct value *self) {
  return value_get_kind(self) == VALUE_INTEGER;
}

bool value_is_real(const struct value *self) {
  return value_get_kind(self) == VALUE_REAL;
}

bool value_is_custom(const struct value *self) {
  return value_get_kind(self) == VALUE_CUSTOM;
}


void value_set_nil(struct value *self) {
  self->kind = VALUE_NIL;
  self->as.custom = NULL;
}

void value_set_boolean(struct value *self, bool val) {
  self->kind = VALUE_BOOLEAN;
  self->as.boolean = val;
}

void value_set_integer(struct value *self, int64_t val) {
  self->kind = VALUE_INTEGER;
  self->as.integer = val;
}

void value_set_real(struct value *self, double val) {
  self->kind = VALUE_REAL;
  self->as.real = val;
}

void value_set_custom(struct value *self, void *val) {
  self->kind = VALUE_CUSTOM;
  self->as.custom = val;
}


bool value_get_boolean(const struct value *self) {
  assert(value_get_kind(self) == VALUE_BOOLEAN);
  return self->as.boolean;
}

int64_t value_get_integer(const struct value *self) {
  assert(value_get_kind(self) == VALUE_INTEGER);
  return self->as.integer;
}

double value_get_real(const struct value *self) {
  assert(value_get_kind(self) == VALUE_REAL);
  return self->as.real;
}

void *value_get_custom(const struct value *self) {
  assert(value_get_kind(self) == VALUE_CUSTOM);
  return self->as.custom;
}


struct value value_make_nil() {
  struct value res;
  value_set_nil(&res);
  return res;
}

struct value value_make_boolean(bool val) {
  struct value res;
  value_set_boolean(&res, val);
  return res;
}

struct value value_make_integer(int64_t val) {
  struct value res;
  value_set_integer(&res, val);
  return res;
}

struct value value_make_real(double val) {
  struct value res;
  value_set_real(&res, val);
  return res;
}

struct value value_make_custom(void *val) {
  struct value res;
  value_set_custom(&res, val);
  return res;
}



/*
 * hashtable
 */


size_t str_length(const char *str)
{
  if (str == NULL)
  { // vérifie que str est un pointeur NULL ou non
    return 0;
  }
  size_t i = 0;
  while (str[i] != '\0')
  {
    ++i;
  }
  return i;
}

void hashtable_create(struct hashtable *self){
  self->size = HASHTABLE_INITIAL_SIZE;
  self->count = 0;
  self->buckets = calloc(self->size, sizeof(struct bucket));
}

bool bucket_empty(const struct bucket *self){
  return (self->key == NULL && value_is_nil(&self->value) && self->next == NULL);
}

void hashtable_destroy(struct hashtable *self){
  for(size_t i = 0; i < self->size; ++i){
    struct bucket *current = self->buckets[i];
    while(current != NULL){
      struct bucket *next = current->next;
      free(current->key);
      free(current);
      current = next;
    }
  }
  free(self->buckets);
}

size_t hashtable_get_count(const struct hashtable *self) {
  return self->count;
}

size_t hashtable_get_size(const struct hashtable *self) {
  return self->size;
}

size_t hash(const char *key){
  size_t hash = 14959810393466560374u;
  size_t prime = 10995116282114u;
  for(size_t i = 0; key[i] != '\0'; ++i){
    hash ^= key[i];
    hash *= prime;
  }
  return hash;
}

bool hashtable_insert(struct hashtable *self, const char *key, struct value val){

  size_t key_hash = hash(key);
  size_t index = key_hash % self->size;
  struct bucket *current = self->buckets[index];              //on récupère le bucket courant à l'indice de hachage et on va 
  while(current != NULL){                                     //parcourir la liste tant que le noeud courant n'est pas NULL
    if(strcmp(current->key, key) == 0){                       //si la clé est déjà présente on a juste a modifié la valeur correspond à la clé
      current->value = val;
      return false;
    }
    current = current->next; 
  }

  char *n_key = malloc((str_length(key) + 1) * sizeof(char)); //sinon on va initialisé le noeud avec la clé, la valeur est mettre le suivant à NULL
  strcpy(n_key, key);

  current = malloc(sizeof(struct bucket));
  current->key = n_key;
  current->value = val;
  current->next = self->buckets[index];
  self->buckets[index] = current;
  ++self->count;

  if((double)(self->count) / self->size > 1.0 / 2){           //on va effectuer un rehash si la compression est supérieur à 0.5
    hashtable_rehash(self);
  }

  return true;
}

bool hashtable_remove(struct hashtable *self, const char *key){
  size_t key_hash = hash(key);
  size_t index = key_hash % self->size;
  struct bucket *current = self->buckets[index];  //on récupère le bucket courant à l'indice de hachage et on va parcourir la liste
  struct bucket *prev = NULL;                     //tant que le noeud n'est pas NULL
  while(current != NULL){
    if(strcmp(current->key, key) == 0){           //si la clé est égal à la clé courante alors
      if(prev != NULL){                           //si prev est non NULL donc on est après le debut de la liste donc le suivant de prev est égal au suivant du courant
        prev->next = current->next;
      }else{                                      //sinon on est au debut de la liste est donc on met le suivant du debut de la liste au suivant du courant
        self->buckets[index] = current->next;
      }
      free(current->key);
      free(current);
      --self->count;
      return true;
    }
    prev = current;
    current = current->next;
  }
  return false;                                 
}

bool hashtable_contains(const struct hashtable *self, const char *key){
  size_t key_hash = hash(key);
  size_t index = key_hash % self->size;
  struct bucket *current = self->buckets[index];  //on récupère le bucket courant à l'indice de hachage et on va parcourir la liste 
  while(current != NULL){
    if(strcmp(current->key, key) == 0){           //si la clé est présente on va retourner vrai
      return true;
    }
    current = current->next;
  }
  return false;
}

void hashtable_rehash(struct hashtable *self){
  size_t old_size = self->size;
  size_t new_size = old_size * 2; //on augmente la taille de 2

  struct bucket **new_buckets = calloc(new_size, sizeof(struct bucket *)); //on initialise le nouveau tableau de bucket à la nouvelle taille

  for(size_t i = 0; i < old_size; ++i){         //on va effectuer une boucle avec la taille de l'ancien tableau
    struct bucket *current = self->buckets[i];  //on va recuperer le bucket de l'indice i
    while(current != NULL){                     //tant que le bucket courant n'est pas NULL 
      struct bucket *next = current->next;      //on récuperer le noeud suivant
      char *key = current->key;                 //initialise la clé au bon indice
      size_t key_hash = hash(key);                
      size_t index = key_hash % new_size;       //recalculer le nouvel indice de hachage

      current->next = new_buckets[index];        
      new_buckets[index] = current;             //on va mettre le noeud courant dans le nouveau tableau à l'indice calculer précédemment

      current = next;
    }
  }

  free(self->buckets);
  self->buckets = new_buckets;
  self->size = new_size;
}

void hashtable_set_nil(struct hashtable *self, const char *key) {
  hashtable_insert(self, key, value_make_nil());
}

void hashtable_set_boolean(struct hashtable *self, const char *key, bool val) {
  hashtable_insert(self, key, value_make_boolean(val));
}

void hashtable_set_integer(struct hashtable *self, const char *key, int64_t val) {
  hashtable_insert(self, key, value_make_integer(val));
}

void hashtable_set_real(struct hashtable *self, const char *key, double val) {
  hashtable_insert(self, key, value_make_real(val));
}

void hashtable_set_custom(struct hashtable *self, const char *key, void *val) {
  hashtable_insert(self, key, value_make_custom(val));
}

struct value hashtable_get(struct hashtable *self, const char *key){
  size_t key_hash = hash(key);
  size_t index = key_hash % self->size;
  struct bucket *current = self->buckets[index];
  while(current != NULL){
    if(strcmp(current->key, key) == 0){
      return current->value;
    }
    current = current->next;
  }
  return value_make_nil();
}
