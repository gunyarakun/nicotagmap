#!/usr/bin/env ruby
# encoding: utf-8

# タグ統計 for ニコニコ動画統計クラスタ
require 'uri'
require 'net/https'
require 'pp'

NNODES = 2525

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
puts "#{NNODES} required, actual #{pgs.length}"
$stdout.flush

pgs = pgs[0...NNODES]

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
n = 0
pg_counts = pgs.map {|tag|
  n += 1
  if n % 100 == 0
    puts "#{Time.now.inspect} get_tag_count n: #{n}"
    $stdout.flush
  end
  get_tag_count(tag)
}

n = 0
# output lgl files see:http://bioinformatics.icmb.utexas.edu/lgl/#FileFormat
open('tag_cooccur.lgl', 'w') {|f|
  (2513...pgs.length).each {|i|
    f.puts "# #{pgs[i]}(#{pg_counts[i]})"
    (i...pgs.length).each {|j|
      tag_count = get_tag_count("#{pgs[i]} #{pgs[j]}")
      f.puts "#{pgs[j]}(#{pg_counts[j]}) #{tag_count}" if i != j

      n += 1
      if n % 500 == 0
        puts "#{Time.now.inspect} n: #{n} i: #{i} j: #{j}"
        $stdout.flush
        f.flush
      end
    }
  }
}
