# nicotagmap

## Python

```
# Install python-igraph, not igraph
pip install python-igraph
```

## Calc 2015-12-12 data

```
# search cooccur data to wordnet
# set threshold 0
ruby cooccur_to_wordnet.rb data/tag_cooccur_2015-12-12-15-34.lgl wordnet.lgl 0
# wordnet to cluster
python wordnet_find_community_multilevel.py data.json
```
