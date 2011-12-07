#!/usr/bin/ruby
$KCODE = 'u'

# タグ統計 for ニコニコ動画統計クラスタ
require 'uri'
require 'net/https'
require 'pp'
require 'jcode'

NNODES = 1000

def convert_hiragana_to_katakana(str)
  str.nil? ? nil : str.tr('ぁ-ん', 'ァ-ン')
end

pgs = []
normalize_hash = {}
open("tagstats.txt") {|f|
  f.each_line {|s|
    tag, count = s.split("\t")
    # ひらがな・カタカナ同一視をして同じものを省く
    normalized_tag = convert_hiragana_to_katakana(tag)
    unless normalize_hash.has_key?(normalized_tag)
      pgs.push(tag.tr(' ', '_'))
      normalize_hash[normalized_tag] = true
    end
  }
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

COUNT_REGEX = %r{<total_count>(\d+)</total_count>};
def get_tag_count(tag)
  tag = tag.tr('〜', '～')

  https = Net::HTTP.new('api.nicovideo.jp', 443)
  https.use_ssl = true

  res = https.start {
    response = https.get('/tag.search?limit=0&tag=' + URI.escape(tag), 'User-Agent' => 'nicowiki')
    m = COUNT_REGEX.match(response.body)
    if m
      m[1]
    else
      nil
    end
  }
end

# get counts
pg_counts = pgs.map {|tag|
  get_tag_count(tag)
}

# get matrix
pg_combs.each {|p|
  tag_count = get_tag_count(p.map{|tag| tag.join(' '))
  pg_matrix[pgs.index(p[0])][pgs.index(p[1])] = tag_count
  pg_matrix[pgs.index(p[1])][pgs.index(p[0])] = tag_count
}

# output lgl files see:http://bioinformatics.icmb.utexas.edu/lgl/#FileFormat
open('tag_cooccur-1000.lgl', 'w') {|f|
  (0...pgs.length).each {|i|
    f.puts "# #{pgs[i]}(#{pg_counts[i]})"
    (i...pgs.length).each {|j|
      f.puts "#{pgs[j]}(#{pg_counts[j]}) #{pg_matrix[i][j]}" if i != j
    }
  }
}
