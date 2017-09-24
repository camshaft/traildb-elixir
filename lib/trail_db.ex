defmodule TrailDB do
  def stream!(root, fields) do
    %__MODULE__.Stream{root: root, fields: fields}
  end
end
