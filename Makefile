CC = gcc 
CCFLAGS = -Wall -Wno-parentheses -Wno-sign-compare -Wno-unknown-pragmas

PROGS = annotate_loci \
        count2tpm \
        expression2gct \
	extract_sequence \
        make_cls \
	make_design_contrast_matrix \
	mean \
        merge_fastq \
	merge_gct \
	minmax_gct \
	reorder_gct \
	replace_header_gct \
	sort_gct \
	subset_gct

B = ./bin
O = ./obj
C = ./csrc
D = ./doc
K = /usr/bios-to-go/kern
S = ./scripts

all: $B $D $(PROGS) $D/tools.md

clean:
	for s in $(PROGS) ; do \
		/bin/rm -f $B/$$s ; \
	done

bin:
	mkdir -p $B


# C programs and scripts 
annotate_loci: $C/annotate_loci.c $K/plabla.c $K/linestream.c $K/rofutil.c \
	$K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $B/annotate_loci
	$(CC) $(CCFLAGS) $C/annotate_loci.c -o $B/annotate_loci $K/plabla.c $K/linestream.c \
	$K/rofutil.c $K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -I$K

expression2gct: $C/expression2gct.c $K/plabla.c $K/linestream.c $K/rofutil.c \
	$K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $B/expression2gct
	$(CC) $(CCFLAGS) $C/expression2gct.c -o $B/expression2gct $K/plabla.c $K/linestream.c \
	$K/rofutil.c $K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -I$K

extract_sequence: $C/extract_sequence.c $K/plabla.c $K/linestream.c $K/rofutil.c \
	$K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $B/extract_sequence
	$(CC) $(CCFLAGS) $C/extract_sequence.c -o $B/extract_sequence $K/plabla.c $K/linestream.c \
	$K/rofutil.c $K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -I$K

count2tpm: $C/count2tpm.c $K/plabla.c $K/linestream.c $K/rofutil.c $K/array.c \
	$K/format.c $K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $(B)/count2tpm
	$(CC) $(CCFLAGS) $C/count2tpm.c -o $B/count2tpm $K/plabla.c $K/linestream.c $K/rofutil.c \
	$K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -lm -I$K

make_cls: $C/make_cls.c $K/plabla.c $K/linestream.c $K/rofutil.c $K/array.c $K/format.c \
	$K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $B/make_cls
	$(CC) $(CCFLAGS) $C/make_cls.c -o $B/make_cls $K/plabla.c $K/linestream.c $K/rofutil.c $K/array.c \
	$K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -I$K

make_design_contrast_matrix: $C/make_design_contrast_matrix.c $K/plabla.c $K/linestream.c $K/rofutil.c $K/array.c $K/format.c \
	$K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $B/make_design_contrast_matrix
	$(CC) $(CCFLAGS) $C/make_design_contrast_matrix.c -o $B/make_design_contrast_matrix $K/plabla.c $K/linestream.c $K/rofutil.c $K/array.c \
	$K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -I$K

mean: $C/mean.c $K/plabla.c $K/linestream.c $K/rofutil.c $K/array.c \
	$K/format.c $K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $(B)/mean
	$(CC) $(CCFLAGS) $C/mean.c -o $B/mean $K/plabla.c $K/linestream.c $K/rofutil.c \
	$K/array.c $K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -lm -I$K

merge_fastq: $C/merge_fastq.c $K/plabla.c $K/linestream.c $K/rofutil.c $K/format.c $K/array.c \
	$K/log.c $K/arg.c $K/hlrmisc.c
	@-/bin/rm -f $B/merge_fastq
	$(CC) $(CCFLAGS) $C/merge_fastq.c -o $B/merge_fastq $K/plabla.c $K/linestream.c $K/rofutil.c $K/array.c \
	$K/format.c $K/log.c $K/arg.c $K/hlrmisc.c -I$K

merge_gct: $S/merge_gct.sh
	cp -p $S/merge_gct.sh $B/merge_gct
	chmod +x $B/merge_gct

minmax_gct: $S/minmax_gct.sh
	cp -p $S/minmax_gct.sh $B/minmax_gct
	chmod +x $B/minmax_gct

reorder_gct: $S/minmax_gct.sh
	cp -p $S/reorder_gct.sh $B/reorder_gct
	chmod +x $B/reorder_gct

replace_header_gct: $S/replace_header_gct.sh
	cp -p $S/replace_header_gct.sh $B/replace_header_gct
	chmod +x $B/replace_header_gct

sort_gct: $S/sort_gct.sh
	cp -p $S/sort_gct.sh $B/sort_gct
	chmod +x $B/sort_gct

subset_gct: $S/subset_gct.sh
	cp -p $S/subset_gct.sh $B/subset_gct
	chmod +x $B/subset_gct

# Make documentation
doc: $B $S/make_doc.sh
	mkdir -p $D && bash $S/make_doc.sh -b $B -o $D/tools.md
