#!/usr/bin/env ruby
# encoding: utf-8
require 'pp'

if ARGV.length != 3
  puts "./calc_wordnet.rb in_tag_cooccur.lgl out_tag_wordnet.lgl threshold"
  exit(1)
end

infile = ARGV[0]
outfile = ARGV[1]
threshold = ARGV[2].to_i

pgs = []
pg_matrix = []
cnt = 0
i = -1
File.open(infile, 'r') {|f|
  f.each_line do |l|
    l.chomp!
    cnt += 1
    puts cnt if cnt % 1000 == 0
    spl = l.split(' ')
    if spl[0] == '#'
      i += 1
      spl.shift
    end
    if (index = pgs.index(spl[0])).nil?
      pgs << spl[0]
      pg_matrix[pgs.length - 1] = [] unless pg_matrix[pgs.length - 1]
      if spl[1]
        cooccur = spl[1].to_i
        pg_matrix[i][pgs.length - 1] = cooccur
        pg_matrix[pgs.length - 1][i] = cooccur
      else
        pg_matrix[i][i] = 0
      end
    else
      if spl[0]
        cooccur = spl[1].to_i
        pg_matrix[i][index] = cooccur
        pg_matrix[index][i] = cooccur
      else
        pg_matrix[i][i] = 0
      end
    end
  end
}

# calc Si
sg = 0
S = (0...pgs.length).map {|i|
  si = 0
  (0...pgs.length).each {|j|
    si += pg_matrix[i][j]
  }
  sg += si
  si
}

E = (0...pgs.length).map {|i|
  (0...pgs.length).map {|j|
    (S[i] * S[j]).to_f / sg
  }
}

C2 = (0...pgs.length).map {|i|
  (0...pgs.length).map {|j|
    (i == j or pg_matrix[i][j] < E[i][j]) ? 0.0 : (pg_matrix[i][j] - E[i][j]) ** 2 / E[i][j]
  }
}

#open('tag_chi2.lgl', 'w') {|f|
#  (0...pgs.length).each {|i|
#    f.puts "# #{pgs[i]}"
#    (i...pgs.length).each {|j|
#      f.puts "#{pgs[j]} #{C2[i][j]}" if i != j
#    }
#  }
#}

open(outfile, 'w') {|f|
  (0...pgs.length).each {|i|
    f.puts "# #{pgs[i]}"
    (i...pgs.length).each {|j|
      f.puts "#{pgs[j]}" if i != j and C2[i][j] > threshold
    }
  }
}
