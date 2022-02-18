# # with zoom
python3 ~/mkplot/mkplot.py -l -p cactus -b pdf --save-to plots/cactus-all-approaches.pdf --shape squared --ylabel="Time (s)" --xlabel=Instances --timeout=1800 --ymax=1800 --xmax=2150 --xmin=1850 --shape=long csvs-plots/all-approaches.csv
# # without zoom
python3 ~/mkplot/mkplot.py -l -p cactus -b pdf --save-to plots/cactus-all-approaches-not-zoomed.pdf --shape squared --ylabel="Time (s)" --xlabel=Instances --timeout=1800 --ymax=1800 --xmax=2150 csvs-plots/all-approaches.csv

# scatter msu3 vs user tags
python3 ~/mkplot/mkplot.py -l -p scatter -b pdf --save-to plots/scatter-msu3-usertags-cores.pdf --shape squared --ylog --xlog --xmin=0.1 --ymin=0.1 --timeout=1800 csvs-plots/msu3-vs-user-tags-cores.csv
# scatter res vs user tags
python3 ~/mkplot/mkplot.py -l -p scatter -b pdf --save-to plots/scatter-res-usertags-cores.pdf --shape squared --ylog --xlog --xmin=0.1 --ymin=0.1 --timeout=1800 csvs-plots/res-vs-user-tags-cores.csv
