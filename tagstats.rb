#!/usr/bin/ruby
$KCODE = 'u'
# タグ統計 for ニコニコ動画統計クラスタ
$: << '../cgis/'
require 'cgi/util'
require 'cgi/db'
require 'config.honban.rb'
require 'pp'

NNODES = 1000

# 上位NNODESを取得

slave_db = SlaveDB.new(CONF)
# NNODESの1.1倍取得。なぜならひらがな・カタカナ同一視をしていないから。
pgs = slave_db.select('SELECT (SELECT pg_title FROM page AS pg WHERE pg.pg_id = t1.ntc_pg_id) AS title FROM nicotagcount AS t1 WHERE ntc_date = (SELECT MAX(ntc_date) FROM nicotagcount AS t2 WHERE t1.ntc_pg_id = t2.ntc_pg_id) ORDER BY ntc_cnt DESC LIMIT ?', (NNODES * 1.1).to_i).map{|i| i[0].tr(' ', '_')}
# ひらがな・カタカナ同一視をして同じものを省く
temp_hash = {}
pgs.reject! {|p|
  np = Util.convert_hiragana_to_katakana(p)
  if temp_hash.has_key?(np)
    true
  else
    temp_hash[np] = true
    false
  end
}
pgs = pgs[0...NNODES]

class Array
  def combination(num)
    return [] if num < 1 || num > size
    return map{|e| [e] } if num == 1
    tmp = self.dup
    self[0, size - (num - 1)].inject([]) do |ret, e|
      tmp.shift
      ret += tmp.combination(num - 1).map{|a| a.unshift(e) }
    end
  end
end

pg_combs = pgs.combination(2)
pg_matrix = (0...pgs.length).map {|i|
  (0...pgs.length).map {|j|
    0
  }
}

# get counts
pg_counts = pgs.map {|p|
  NicoVideo.get_tag_count(p)
}

# get matrix
pg_combs.each {|p|
  tag_count = NicoVideo.get_tag_count_raw(p.join(' '))
  pg_matrix[pgs.index(p[0])][pgs.index(p[1])] = tag_count
  pg_matrix[pgs.index(p[1])][pgs.index(p[0])] = tag_count
}

# output lgl files see:http://bioinformatics.icmb.utexas.edu/lgl/#FileFormat
open('tag_cooccur.lgl', 'w') {|f|
  (0...pgs.length).each {|i|
    f.puts "# #{pgs[i]}(#{pg_counts[i]})"
    (i...pgs.length).each {|j|
      f.puts "#{pgs[j]}(#{pg_counts[j]}) #{pg_matrix[i][j]}" if i != j
    }
  }
}

