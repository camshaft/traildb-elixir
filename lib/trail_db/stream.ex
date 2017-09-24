defmodule TrailDB.Stream do
  defstruct [root: nil, fields: []]

  defimpl Collectable do
    alias TrailDB.Constructor

    def into(%{root: root, fields: fields} = stream) do
      case Constructor.open(root, fields) do
        {:ok, db} ->
          {:ok, itr(db, stream)}
        {:error, reason} ->
          raise ArgumentError, to_string(reason)
      end
    end

    defp itr(db, %{fields: fields} = stream) do
      fn
        :ok, {:cont, {uuid, timestamp, values}} when is_map(values) ->
          values = Enum.map(fields, &to_string(Map.get(values, &1, "")))
          Constructor.add(db, uuid, timestamp, values)
        :ok, {:cont, {uuid, timestamp, values}} when is_list(values) ->
          values = Enum.map(values, &to_string/1)
          Constructor.add(db, uuid, timestamp, values)
        :ok, :done ->
          :ok = Constructor.finalize(db)
          stream
        :ok, :halt ->
          # Don't finalize
          :ok
      end
    end
  end

  # TODO implement Enumerable
end
