#!/usr/bin/env ruby
# encoding: utf-8

# タグ統計 for ニコニコ動画統計クラスタ
require 'uri'
require 'net/https'
require 'pp'

if ARGV.length != 3
  puts "./#{$PROGRAM_NAME} infile.txt outfile.lgl node_count"
  exit(1)
end

infile = ARGV[0]
outfile = ARGV[1]
nnodes = ARGV[2].to_i

def convert_hiragana_to_katakana(str)
  str.nil? ? nil : str.tr('ぁ-ん', 'ァ-ン')
end

pgs = []
pg_counts = []
normalize_hash = {}
open(infile) {|f|
  f.each_line {|s|
    tag, count = s.split("\t")
    # ひらがな・カタカナ同一視をして同じものを省く
    normalized_tag = convert_hiragana_to_katakana(tag)
    unless normalize_hash.has_key?(normalized_tag)
      pgs.push(tag.tr(' ', '_'))
      pg_counts.push(count.to_i)
      normalize_hash[normalized_tag] = true
    end
  }
}
puts "#{nnodes} required, actual #{pgs.length}"
$stdout.flush

pgs = pgs[0...nnodes]
pg_counts = pg_counts[0...nnodes]

COUNT_REGEX = %r{<total_count>(\d+)</total_count>};
def get_tag_count(tag)
  tag = tag.tr('〜', '～')

  https = Net::HTTP.new('api.nicovideo.jp', 443)
  https.use_ssl = true

  res = https.start {
    response = https.get('/tag.search?limit=0&tag=' + URI.escape(tag), 'User-Agent' => 'nicowiki')
    m = COUNT_REGEX.match(response.body)
    if m
      m[1].to_i
    else
      nil
    end
  }
end

n = 0
# output lgl files see:http://bioinformatics.icmb.utexas.edu/lgl/#FileFormat
open(outfile, 'w') {|f|
  (0...pgs.length).each {|i|
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
