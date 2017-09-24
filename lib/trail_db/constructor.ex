defmodule TrailDB.Constructor do
  @on_load { :__on_load__, 0 }

  app = Mix.Project.config[:app]

  def __on_load__ do
    path = :filename.join(:code.priv_dir(unquote(app)), 'traildb_cons')
    _ = :erlang.load_nif(path, 0)
    :ok
  end

  def open(_root, _field_names) do
    exit(:nif_library_not_loaded)
  end

  def add(_db, _uuid, _timestamp, _fields) do
    exit(:nif_library_not_loaded)
  end

  def finalize(_db) do
    exit(:nif_library_not_loaded)
  end
end
