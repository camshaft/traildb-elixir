defmodule Test.TrailDB do
  use ExUnit.Case

  setup_all do
    data = file("")
    File.rm_rf!(data)
    File.mkdir_p!(data)
    :ok
  end

  test "stream!/2" do
    fields = [
      "fieldA",
      "fieldB",
    ]

    1..1_000
    |> Elixir.Stream.map(fn(i) ->
      uuid = :crypto.strong_rand_bytes(16)
      {uuid, i, Enum.map(fields, fn(_) ->
        :crypto.strong_rand_bytes(20) |> Base.encode64
      end)}
    end)
    |> Enum.into(TrailDB.stream!(file("stream"), fields))

    :ok
  end

  defp file(name) do
    Path.join([__DIR__, ".data", name])
  end
end
