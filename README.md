# Run2 ESD to Run3 AOD converter

This is an helper executable to read Run2 ESD data and pump it to stdout
as a set of Arrow `BatchRecord`s. This is to be coupled with some yet to happen
development in DPL to chunk the stream into messages.

The `scripts/pythonReader.py` python script on the other hand is an initial 
attempt to demonstrate how this can also be processed using python + pandas.

The python example can be run by doing:

```bash
run2ESD2Run3AOD ~/work/active/data/AliESDs_15000246751039.3412.root | scripts/pythonReader.py
```

and then you should find a few `figure*.pdf` files with some random plots in your current path.

In order to validate the conversion you can use the `validateAODStream` helper.

# Updating to a given version of AliRoot / O2

The converter embeds a copy of the relevant AliRoot files to be able to read ESD event
content. It also contains a copy of the O2 arrow based utilities. In order to update to the
latest greatest version you need to:

```
git clone https://github.com/AliceO2Group/AliceO2 O2
git clone https://github.com/AliceO2Group/Run2ESDConverter O2
git clone https://github.com/alisw/AliRoot
cd Run2ESDConverter
rm -rf Run2DataModel/src
./copyArrowDataModel.sh
./copyAliESDEvent.sh
```
