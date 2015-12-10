#!/usr/bin/env ruby
# encoding: utf-8

# タグリストの最新の検索件数を取得する

if ARGV.length != 2
  puts "#{$PROGRAM_NAME} input.txt output.txt"
  exit(1)
end

infile, outfile = ARGV

require 'uri'
require 'net/https'
require 'pp'

def convert_hiragana_to_katakana(str)
  str.nil? ? nil : str.tr('ぁ-ん', 'ァ-ン')
end

tags = []
normalize_hash = {}
open(infile) {|f|
  f.each_line {|s|
    tag, count = s.split("\t")
    # ひらがな・カタカナ同一視をして同じものを省く
    normalized_tag = convert_hiragana_to_katakana(tag)
    unless normalize_hash.has_key?(normalized_tag)
      tags.push({
        :original => tag,
        :original_count => count.to_i,
        :normalized => tag.tr(' ', '_'),
      })
      normalize_hash[normalized_tag] = true
    end
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
      m[1].to_i
    else
      nil
    end
  }
end

# get counts
n = 0
pg_counts = tags.map {|tag|
  n += 1
  if n % 100 == 0
    puts "#{Time.now.inspect} get_tag_count n: #{n}"
    $stdout.flush
  end
  tag[:count] = get_tag_count(tag[:normalized])
}

tags.sort_by! {|v| v[:count] }.reverse!

open(outfile, 'w') {|f|
  tags.each {|tag|
    f.puts "#{tag[:original]}\t#{tag[:count]}"
  }
}

