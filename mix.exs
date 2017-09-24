defmodule Mix.Tasks.Compile.TrailDb do
  def run(_) do
    case System.cmd("make", ["priv/traildb_cons.so"], stderr_to_stdout: true) do
      {result, 0} ->
        IO.binwrite result
        :ok
      {result, status} ->
        IO.binwrite result
        raise CompileError, "status: #{status}"
    end
  end
end

defmodule TrailDB.Mixfile do
  use Mix.Project

  @version "0.1.0"

  def project do
    [app: :trail_db,
     version: @version,
     elixir: ">= 1.2.0",
     compilers: [:trail_db, :elixir, :app],
     deps: deps()]
  end

  def application do
    []
  end

  defp deps do

  end
end
