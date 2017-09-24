#include "erl_nif.h"
#include <string.h>
#include <stdbool.h>
#include <traildb.h>

ErlNifResourceType *CONS_RES_TYPE;

typedef struct
{
  tdb_cons* constructor;
  unsigned size;
  bool finalized;
} TrailDBConstructor;

ERL_NIF_TERM mk_atom(ErlNifEnv* env, const char* atom)
{
  ERL_NIF_TERM ret;

  if (!enif_make_existing_atom(env, atom, &ret, ERL_NIF_LATIN1)) {
    return enif_make_atom(env, atom);
  }

  return ret;
}

ERL_NIF_TERM mk_error(ErlNifEnv* env, const char* error) {
  return enif_make_tuple2(env, mk_atom(env, "error"), mk_atom(env, error));
}

ERL_NIF_TERM mk_ok(ErlNifEnv* env, ERL_NIF_TERM term) {
  return enif_make_tuple2(env, mk_atom(env, "ok"), term);
}

char* read_iolist(ErlNifEnv* env, ERL_NIF_TERM term) {
  ErlNifBinary bin;
  if (!enif_inspect_iolist_as_binary(env, term, &bin)) {
    return "";
  }
  return (char *) bin.data;
}

const char** read_fields(ErlNifEnv* env, ERL_NIF_TERM list, unsigned len) {
  const char **fields = malloc(sizeof(char*) * len);

  ERL_NIF_TERM field;
  unsigned i = 0;
  while (enif_get_list_cell(env, list, &field, &list) != 0) {
    fields[i] = read_iolist(env, field);
    i++;
  }

  return fields;
}

static ERL_NIF_TERM open(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  char *root = read_iolist(env, argv[0]);
  // TODO verify root size is > 0

  unsigned len;
  enif_get_list_length(env, argv[1], &len);
  const char **fields = read_fields(env, argv[1], len);

  tdb_cons *cons = tdb_cons_init();
  tdb_error err = tdb_cons_open(cons, root, fields, len);

  free(fields);

  if (err) {
    return mk_error(env, tdb_error_str(err));
  }

  TrailDBConstructor* res = enif_alloc_resource(CONS_RES_TYPE, sizeof(TrailDBConstructor));
  if (res == NULL) return enif_make_badarg(env);
  ERL_NIF_TERM ret = enif_make_resource(env, res);
  enif_release_resource(res);

  res->constructor = cons;
  res->size = len;
  res->finalized = false;

  return mk_ok(env, ret);
}

static ERL_NIF_TERM add(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  TrailDBConstructor *res;
  if (!enif_get_resource(env, argv[0], CONS_RES_TYPE, (void**) &res)) {
    return mk_error(env, "invalid_constructor");
  }

  if (res->finalized) {
    return mk_error(env, "already_finalized");
  }

  ErlNifBinary uuid;
  if (!enif_inspect_iolist_as_binary(env, argv[1], &uuid)) {
    return mk_error(env, "invalid_uuid");
  }

  if (uuid.size != (sizeof(char) * 16)) {
    return mk_error(env, "invalid_uuid_size");
  }

  ErlNifUInt64 timestamp;
  enif_get_uint64(env, argv[2], &timestamp);

  unsigned size = res->size;
  const char **values = malloc(sizeof(char*) * size);
  uint64_t *lengths = malloc(sizeof(uint64_t) * size);

  ERL_NIF_TERM list = argv[3];
  ERL_NIF_TERM field;
  ErlNifBinary field_bin;
  unsigned i = 0;
  while (i < size && enif_get_list_cell(env, list, &field, &list) != 0) {
    if (enif_inspect_iolist_as_binary(env, field, &field_bin)) {
      values[i] = (char *) field_bin.data;
      lengths[i] = field_bin.size;
    }
    i++;
  }
  // fill in any missing values
  while (i < size) {
    values[i] = "";
    lengths[i] = 0;
    i++;
  }

  tdb_error err = tdb_cons_add(res->constructor, uuid.data, timestamp, values, lengths);

  free(values);
  free(lengths);

  if (err) {
    return mk_error(env, tdb_error_str(err));
  }

  return mk_atom(env, "ok");
}

static ERL_NIF_TERM finalize(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  tdb_error err;

  TrailDBConstructor *res;
  if (!enif_get_resource(env, argv[0], CONS_RES_TYPE, (void**) &res)) {
    return mk_error(env, "invalid_constructor");
  }

  if (res->finalized) {
    return mk_error(env, "already_finalized");
  }

  if ((err = tdb_cons_finalize(res->constructor))) {
    return mk_error(env, tdb_error_str(err));
  }

  res->finalized = true;

  return mk_atom(env, "ok");
}

static ErlNifFunc nif_funcs[] =
{
  {"open", 2, open, ERL_NIF_DIRTY_JOB_IO_BOUND},
  {"add", 4, add, 0},
  {"finalize", 1, finalize, ERL_NIF_DIRTY_JOB_IO_BOUND},
};

void cons_res_destructor(ErlNifEnv *env, void *res) {
  tdb_cons_close(((TrailDBConstructor *) res)->constructor);
}

int load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info) {
  int flags = ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER;
  CONS_RES_TYPE =
    enif_open_resource_type(env, "Elixir.TrailDB.Constructor", "TrailDBConstructor", cons_res_destructor, flags, NULL);
  return 0;
}

ERL_NIF_INIT(Elixir.TrailDB.Constructor, nif_funcs, &load, NULL, NULL, NULL)
