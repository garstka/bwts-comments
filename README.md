
# bwts-comments

Comments for BWTS. 

## BWTS

BWTS by David Scott is a bijective variant of the Burrows-Wheeler transform,
which is a reversible transform that rearranges a character string into runs of
similar characters. BWT requires storing an EOF character or its offset, BWTS does not. 

## bwts-comments

I found the original implementation of BWTS to be rather cryptic, so I added
comments and introduced minor cosmetic changes that make the code easier to
understand. Perhaps someone will find this helpful, I know I did.

## Usage

The original bwts.cpp and unbwts.cpp can be found in the archive original/bwts.zip.

    bwts in.bin out.bin
    unbwts out.bin back.bin


## Links

#### General
https://en.wikipedia.org/wiki/Burrows–Wheeler_transform

#### How it works
https://github.com/zephyrtronium/bwst/blob/master/README.markdown

#### Formal proof of correctness
https://arxiv.org/abs/0908.0239
                                                                                