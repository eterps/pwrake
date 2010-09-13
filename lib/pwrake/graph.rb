module Pwrake

  class Graphviz

    def initialize
      @nodes = []
      @edges = []
      @tag_hash = {}
      @count = 0
      @traced = {}
    end
    
    def trace( name = :default )
      push_node( name )
      task = Rake.application[name]      
      @traced[task] = true
      task.prerequisites.each_with_index do |prereq_name,i|
        prereq = Rake.application[prereq_name]
        if i < 150
          if ! @traced[prereq]
            trace( prereq_name )
          end
          push_edge( prereq_name, name )
        end
      end
    end

    def trim( name )
      name = name.to_s
      name = File.basename(name)
      name.sub(/H\d+/,'').sub(/object\d+/,"")
    end

    def push_node( name )
      if @tag_hash[name].nil?
        tag = "T#{@count}"
        @tag_hash[name] = tag
        @nodes.push "#{tag} [label=\"#{trim(name)}\", style=filled, fillcolor=magenta];"
        @count += 1
      end
    end

    def push_edge( prereq_name, name )
      @edges.push "#{@tag_hash[prereq_name]} -> #{@tag_hash[name]};"
    end

    def write(file)
      open(file, "w") do |w|
        #w.puts "digraph sample {\ngraph [size=\"12,100\",ranksep=1.5,nodesep=0.2];"
        w.puts "digraph sample {"
        w.puts "graph [size=\"70,70\", rankdir=LR];"
        @nodes.each do |x|
          w.puts x
        end
        @edges.each do |x|
          w.puts x
        end
        w.puts "}"
      end
    end
  end
end
