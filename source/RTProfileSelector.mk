##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Release
ProjectName            :=RTProfileSelector
ConfigurationName      :=Release
WorkspacePath          := "/home/mc/Development/Code/bitbucket/rtprofileselector/source"
ProjectPath            := "/home/mc/Development/Code/bitbucket/rtprofileselector/source"
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=mc
Date                   :=11/20/14
CodeLitePath           :="/home/mc/.codelite"
LinkerName             :=/usr/bin/g++-4.8 
SharedObjectLinkerName :=/usr/bin/g++-4.8 -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)NDEBUG 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="RTProfileSelector.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++-4.8 
CC       := /usr/bin/gcc-4.8 
CXXFLAGS :=  -O2 -Wall -std=c++0x $(Preprocessors)
CFLAGS   :=  -O2 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as 


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/RTProfileSelector.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

$(IntermediateDirectory)/.d:
	@test -d ./Release || $(MakeDirCommand) ./Release

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/RTProfileSelector.cpp$(ObjectSuffix): RTProfileSelector.cpp $(IntermediateDirectory)/RTProfileSelector.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/mc/Development/Code/bitbucket/rtprofileselector/source/RTProfileSelector.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/RTProfileSelector.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/RTProfileSelector.cpp$(DependSuffix): RTProfileSelector.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/RTProfileSelector.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/RTProfileSelector.cpp$(DependSuffix) -MM "RTProfileSelector.cpp"

$(IntermediateDirectory)/RTProfileSelector.cpp$(PreprocessSuffix): RTProfileSelector.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/RTProfileSelector.cpp$(PreprocessSuffix) "RTProfileSelector.cpp"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) $(IntermediateDirectory)/RTProfileSelector.cpp$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/RTProfileSelector.cpp$(DependSuffix)
	$(RM) $(IntermediateDirectory)/RTProfileSelector.cpp$(PreprocessSuffix)
	$(RM) $(OutputFile)
	$(RM) ".build-release/RTProfileSelector"


