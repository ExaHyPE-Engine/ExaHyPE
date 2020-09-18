AUTOMAKE_OPTIONS = subdir-objects

if UseVTK
  bin_PROGRAMS = convert convert_debug
endif


 
  
#
# find . -name 'libvtk*.so*' -exec nm --print-file-name --defined-only --dynamic {} \; | grep "vtkFilter"
#  
if VTK_LIBRARY_VERSION_7
VTK_REQUIRED_LIBS     = -lvtkalglib$(VTK_LIBRARY_SUFFIX) \
						-lvtkChartsCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkChartsCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkChartsCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonColor$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonColorPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonColorTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonComputationalGeometry$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonComputationalGeometryPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonComputationalGeometryTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonDataModel$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonDataModelPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonDataModelTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonExecutionModel$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonExecutionModelPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonExecutionModelTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonMath$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonMathPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonMathTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonMisc$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonMiscPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonMiscTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonSystem$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonSystemPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonSystemTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonTransforms$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonTransformsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkCommonTransformsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkDICOMParser$(VTK_LIBRARY_SUFFIX) \
						-lvtkDomainsChemistry$(VTK_LIBRARY_SUFFIX) \
						-lvtkDomainsChemistryOpenGL2$(VTK_LIBRARY_SUFFIX) \
						-lvtkDomainsChemistryOpenGL2Python36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkDomainsChemistryPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkDomainsChemistryTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkexoIIc$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersAMR$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersAMRPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersAMRTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersExtraction$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersExtractionPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersExtractionTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersFlowPaths$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersFlowPathsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersFlowPathsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGeneral$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGeneralPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGeneralTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGeneric$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGenericPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGenericTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGeometry$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGeometryPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersGeometryTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersHybrid$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersHybridPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersHybridTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersHyperTree$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersHyperTreePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersHyperTreeTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersImaging$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersImagingPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersImagingTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersModeling$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersModelingPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersModelingTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallel$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelDIY2$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelDIY2Python36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelDIYIITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelFlowPaths$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelFlowPathsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelFlowPathsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelGeometry$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelGeometryPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelGeometryTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelImaging$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelImagingPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelImagingTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelMPI$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelMPIPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelMPITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelStatistics$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelStatisticsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelStatisticsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersParallelTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersPoints$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersPointsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersPointsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersProgrammable$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersProgrammablePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersProgrammableTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersPython$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersPythonPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersReebGraph$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersReebGraphPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersReebGraphTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSelection$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSelectionPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSelectionTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSMP$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSMPPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSMPTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSources$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSourcesPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersSourcesTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersStatistics$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersStatisticsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersStatisticsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersTexture$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersTexturePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersTextureTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersVerdict$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersVerdictPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkFiltersVerdictTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkGeovisCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkGeovisCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkGeovisCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingColor$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingColorPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingColorTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingFourier$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingFourierPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingFourierTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingGeneral$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingGeneralPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingGeneralTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingHybrid$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingHybridPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingHybridTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingMath$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingMathPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingMathTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingMorphological$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingMorphologicalPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingMorphologicalTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingSources$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingSourcesPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingSourcesTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingStatistics$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingStatisticsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingStatisticsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingStencil$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingStencilPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkImagingStencilTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisBoostGraphAlgorithms$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisBoostGraphAlgorithmsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisBoostGraphAlgorithmsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisLayout$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisLayoutPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkInfovisLayoutTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionImage$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionImagePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionImageTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionStyle$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionStylePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionStyleTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionWidgets$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionWidgetsPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkInteractionWidgetsTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOAMR$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOAMRPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOAMRTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOEnSight$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOEnSightPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOEnSightTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOExodus$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOExodusPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOExodusTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOExport$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOExportPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOExportTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOFFMPEG$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOFFMPEGPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOFFMPEGTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGDAL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGDALPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGDALTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGeoJSON$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGeoJSONPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGeoJSONTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGeometry$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGeometryPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOGeometryTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOImage$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOImagePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOImageTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOImport$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOImportPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOImportTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOInfovis$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOInfovisPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOInfovisTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOLegacy$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOLegacyPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOLegacyTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOLSDyna$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOLSDynaPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOLSDynaTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMINC$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMINCPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMINCTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMovie$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMoviePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMovieTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMPIImage$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMPIImagePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMPIImageTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMPIParallel$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMPIParallelPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMPIParallelTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMySQL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMySQLPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOMySQLTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIONetCDF$(VTK_LIBRARY_SUFFIX) \
						-lvtkIONetCDFPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIONetCDFTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOODBC$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOODBCPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOODBCTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallel$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelExodus$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelExodusPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelExodusTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelLSDyna$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelLSDynaPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelLSDynaTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelNetCDF$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelNetCDFPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelNetCDFTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelXML$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelXMLPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOParallelXMLTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOPLY$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOPLYPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOPLYTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOPostgreSQL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOPostgreSQLPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOPostgreSQLTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOSQL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOSQLPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOSQLTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOTecplotTable$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOTecplotTablePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOTecplotTableTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOVideo$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOVideoPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOVideoTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOVPIC$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOVPICPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOVPICTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXdmf2$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXdmf2Python36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXdmfIITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXML$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXMLParser$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXMLParserPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXMLParserTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXMLPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkIOXMLTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkLocalExample$(VTK_LIBRARY_SUFFIX) \
						-lvtkLocalExampleTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkmetaio$(VTK_LIBRARY_SUFFIX) \
						-lvtkParallelCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkParallelCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkParallelCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkParallelMPI4Py$(VTK_LIBRARY_SUFFIX) \
						-lvtkParallelMPI4PyPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkParallelMPI$(VTK_LIBRARY_SUFFIX) \
						-lvtkParallelMPIPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkParallelMPITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkPythonInterpreter$(VTK_LIBRARY_SUFFIX) \
						-lvtkPythonInterpreterPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkPythonInterpreterTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingAnnotation$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingAnnotationPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingAnnotationTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingChemistryOpenGLIITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingContext2D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingContext2DPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingContextIIDTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingContextOpenGL2$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingContextOpenGL2Python36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingContextOpenGLIITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingExternal$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingExternalPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingExternalTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingFreeType$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingFreeTypeFontConfig$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingFreeTypePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingFreeTypeTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingGL2PSOpenGL2$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingGL2PSOpenGL2Python36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingGLtoPSOpenGLIITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingImage$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingImagePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingImageTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLabel$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLabelPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLabelTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLICOpenGL2$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLICOpenGL2Python36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLICOpenGLIITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLOD$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLODPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingLODTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingMatplotlib$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingMatplotlibPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingMatplotlibTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingOpenGL2$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingOpenGL2Python36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingOpenGLIITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingParallel$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingParallelLIC$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingParallelLICPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingParallelLICTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingParallelPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingParallelTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingSceneGraph$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingSceneGraphPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingSceneGraphTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingTkTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolume$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolumeAMR$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolumeAMRPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolumeAMRTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolumeOpenGL2$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolumeOpenGL2Python36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolumeOpenGLIITCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolumePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkRenderingVolumeTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtksys$(VTK_LIBRARY_SUFFIX) \
						-lvtkTestingGenericBridge$(VTK_LIBRARY_SUFFIX) \
						-lvtkTestingIOSQL$(VTK_LIBRARY_SUFFIX) \
						-lvtkTestingRendering$(VTK_LIBRARY_SUFFIX) \
						-lvtkTestingRenderingPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkTestingRenderingTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkverdict$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsContext2D$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsContext2DPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsContextIIDTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsCore$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsCorePython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsCoreTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsGeovis$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsGeovisPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsGeovisTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsInfovis$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsInfovisPython36D$(VTK_LIBRARY_SUFFIX) \
						-lvtkViewsInfovisTCL$(VTK_LIBRARY_SUFFIX) \
						-lvtkVPIC$(VTK_LIBRARY_SUFFIX) \
						-lvtkWrappingJava$(VTK_LIBRARY_SUFFIX) \
						-lvtkWrappingPython36Core$(VTK_LIBRARY_SUFFIX) \
						-lvtkxdmf2$(VTK_LIBRARY_SUFFIX)
endif

if VTK_LIBRARY_VERSION_8
VTK_REQUIRED_LIBS     =  -lvtkIOXML$(VTK_LIBRARY_SUFFIX) \
                         -lvtkCommonExecutionModel$(VTK_LIBRARY_SUFFIX) \
                         -lvtkCommonCore$(VTK_LIBRARY_SUFFIX) \
                         -lvtkCommonDataModel$(VTK_LIBRARY_SUFFIX) \
                         -lvtkFiltersCore$(VTK_LIBRARY_SUFFIX) \
                         -lvtkFiltersGeneral$(VTK_LIBRARY_SUFFIX) \
                         -lvtkIOXMLParser$(VTK_LIBRARY_SUFFIX) \
                         -lvtkIOCore$(VTK_LIBRARY_SUFFIX) \
                         -lvtkCommonSystem$(VTK_LIBRARY_SUFFIX) \
                         -lvtkCommonTransforms$(VTK_LIBRARY_SUFFIX) \
                         -lvtkCommonMisc$(VTK_LIBRARY_SUFFIX) \
                         -lvtkCommonMath$(VTK_LIBRARY_SUFFIX) \
                         -lvtksys$(VTK_LIBRARY_SUFFIX) 
endif


if VTK_LIBRARY_VERSION_9
VTK_REQUIRED_LIBS     =  \
-lvtkChartsCore$(VTK_LIBRARY_SUFFIX)                   -lvtkFiltersPoints$(VTK_LIBRARY_SUFFIX)           -lvtkInteractionImage$(VTK_LIBRARY_SUFFIX)    -lvtkIOImport$(VTK_LIBRARY_SUFFIX)       -lvtkIOTecplotTable$(VTK_LIBRARY_SUFFIX)          -lvtkRenderingLabel$(VTK_LIBRARY_SUFFIX)          -lvtkViewsContext2D$(VTK_LIBRARY_SUFFIX) \
-lvtkCommonColor$(VTK_LIBRARY_SUFFIX)                  -lvtkFiltersAMR$(VTK_LIBRARY_SUFFIX)              -lvtkFiltersProgrammable$(VTK_LIBRARY_SUFFIX)  -lvtkInteractionStyle$(VTK_LIBRARY_SUFFIX)    -lvtkIOInfovis$(VTK_LIBRARY_SUFFIX)      -lvtkIOVeraOut$(VTK_LIBRARY_SUFFIX)                           -lvtkRenderingLOD$(VTK_LIBRARY_SUFFIX)            -lvtkViewsCore$(VTK_LIBRARY_SUFFIX) \
-lvtkCommonComputationalGeometry$(VTK_LIBRARY_SUFFIX)  -lvtkFiltersCore$(VTK_LIBRARY_SUFFIX)             -lvtkFiltersPython$(VTK_LIBRARY_SUFFIX)        -lvtkImagingColor$(VTK_LIBRARY_SUFFIX)          -lvtkInteractionWidgets$(VTK_LIBRARY_SUFFIX)  -lvtkIOLegacy$(VTK_LIBRARY_SUFFIX)       -lvtkIOVideo$(VTK_LIBRARY_SUFFIX)         -lvtkParallelCore$(VTK_LIBRARY_SUFFIX)           -lvtkRenderingOpenGL2$(VTK_LIBRARY_SUFFIX)        -lvtkViewsInfovis$(VTK_LIBRARY_SUFFIX) \
-lvtkCommonCore$(VTK_LIBRARY_SUFFIX)                   -lvtkFiltersExtraction$(VTK_LIBRARY_SUFFIX)       -lvtkFiltersSelection$(VTK_LIBRARY_SUFFIX)     -lvtkImagingCore$(VTK_LIBRARY_SUFFIX)           -lvtkIOAMR$(VTK_LIBRARY_SUFFIX)               -lvtkIOLSDyna$(VTK_LIBRARY_SUFFIX)       -lvtkIOXML$(VTK_LIBRARY_SUFFIX)           -lvtkParallelDIY$(VTK_LIBRARY_SUFFIX)            -lvtkRenderingSceneGraph$(VTK_LIBRARY_SUFFIX)     -lvtkWrappingPythonCore$(VTK_LIBRARY_SUFFIX) \
-lvtkCommonDataModel$(VTK_LIBRARY_SUFFIX)              -lvtkFiltersFlowPaths$(VTK_LIBRARY_SUFFIX)        -lvtkFiltersSMP$(VTK_LIBRARY_SUFFIX)           -lvtkImagingFourier$(VTK_LIBRARY_SUFFIX)        -lvtkIOAsynchronous$(VTK_LIBRARY_SUFFIX)      -lvtkIOMINC$(VTK_LIBRARY_SUFFIX)         -lvtkIOXMLParser$(VTK_LIBRARY_SUFFIX)     -lvtkRenderingUI$(VTK_LIBRARY_SUFFIX)             -lvtkWrappingTools$(VTK_LIBRARY_SUFFIX) \
-lvtkCommonExecutionModel$(VTK_LIBRARY_SUFFIX)         -lvtkFiltersGeneral$(VTK_LIBRARY_SUFFIX)          -lvtkFiltersSources$(VTK_LIBRARY_SUFFIX)       -lvtkImagingGeneral$(VTK_LIBRARY_SUFFIX)        -lvtkIOCityGML$(VTK_LIBRARY_SUFFIX)           -lvtkIOMotionFX$(VTK_LIBRARY_SUFFIX)     -lvtkRenderingVolume$(VTK_LIBRARY_SUFFIX) \
-lvtkCommonMath$(VTK_LIBRARY_SUFFIX)                   -lvtkFiltersGeneric$(VTK_LIBRARY_SUFFIX)          -lvtkFiltersStatistics$(VTK_LIBRARY_SUFFIX)    -lvtkImagingHybrid$(VTK_LIBRARY_SUFFIX)         -lvtkIOCore$(VTK_LIBRARY_SUFFIX)              -lvtkIOMovie$(VTK_LIBRARY_SUFFIX)        -lvtkPythonContext2D$(VTK_LIBRARY_SUFFIX)        -lvtkRenderingVolumeOpenGL2$(VTK_LIBRARY_SUFFIX) \
-lvtkCommonMisc$(VTK_LIBRARY_SUFFIX)                   -lvtkFiltersGeometry$(VTK_LIBRARY_SUFFIX)         -lvtkFiltersTexture$(VTK_LIBRARY_SUFFIX)       -lvtkImagingMath$(VTK_LIBRARY_SUFFIX)           -lvtkIOEnSight$(VTK_LIBRARY_SUFFIX)           -lvtkIONetCDF$(VTK_LIBRARY_SUFFIX)       -lvtklibharu$(VTK_LIBRARY_SUFFIX)         -lvtkPythonInterpreter$(VTK_LIBRARY_SUFFIX)      -lvtkRenderingVtkJS$(VTK_LIBRARY_SUFFIX) \
-lvtkCommonSystem$(VTK_LIBRARY_SUFFIX)                 -lvtkFiltersHybrid$(VTK_LIBRARY_SUFFIX)           -lvtkFiltersTopology$(VTK_LIBRARY_SUFFIX)      -lvtkImagingMorphological$(VTK_LIBRARY_SUFFIX)  -lvtkIOExodus$(VTK_LIBRARY_SUFFIX)            -lvtkIOOggTheora$(VTK_LIBRARY_SUFFIX)    -lvtkRenderingAnnotation$(VTK_LIBRARY_SUFFIX)    \
-lvtkCommonTransforms$(VTK_LIBRARY_SUFFIX)             -lvtkFiltersHyperTree$(VTK_LIBRARY_SUFFIX)        -lvtkFiltersVerdict$(VTK_LIBRARY_SUFFIX)       -lvtkImagingSources$(VTK_LIBRARY_SUFFIX)        -lvtkIOExport$(VTK_LIBRARY_SUFFIX)            -lvtkIOParallel$(VTK_LIBRARY_SUFFIX)     -lvtkRenderingContext2D$(VTK_LIBRARY_SUFFIX)     -lvtksys$(VTK_LIBRARY_SUFFIX) \
-lvtkDICOMParser$(VTK_LIBRARY_SUFFIX)                  -lvtkFiltersImaging$(VTK_LIBRARY_SUFFIX)          -lvtkImagingStatistics$(VTK_LIBRARY_SUFFIX)     -lvtkIOExportGL2PS$(VTK_LIBRARY_SUFFIX)       -lvtkIOParallelXML$(VTK_LIBRARY_SUFFIX)  -lvtkloguru$(VTK_LIBRARY_SUFFIX)          -lvtkRenderingCore$(VTK_LIBRARY_SUFFIX)          -lvtkTestingRendering$(VTK_LIBRARY_SUFFIX) \
-lvtkDomainsChemistry$(VTK_LIBRARY_SUFFIX)             -lvtkFiltersModeling$(VTK_LIBRARY_SUFFIX)         -lvtkGeovisCore$(VTK_LIBRARY_SUFFIX)           -lvtkImagingStencil$(VTK_LIBRARY_SUFFIX)        -lvtkIOExportPDF$(VTK_LIBRARY_SUFFIX)         -lvtkIOPLY$(VTK_LIBRARY_SUFFIX)          -lvtkRenderingFreeType$(VTK_LIBRARY_SUFFIX)      \
-lvtkFiltersParallel$(VTK_LIBRARY_SUFFIX)         -lvtkInfovisCore$(VTK_LIBRARY_SUFFIX)           -lvtkIOGeometry$(VTK_LIBRARY_SUFFIX)          -lvtkIOSegY$(VTK_LIBRARY_SUFFIX)         -lvtkRenderingGL2PSOpenGL2$(VTK_LIBRARY_SUFFIX)  \
-lvtkexodusII$(VTK_LIBRARY_SUFFIX)                     -lvtkFiltersParallelImaging$(VTK_LIBRARY_SUFFIX)  -lvtkInfovisLayout$(VTK_LIBRARY_SUFFIX)         -lvtkIOImage$(VTK_LIBRARY_SUFFIX)             -lvtkIOSQL$(VTK_LIBRARY_SUFFIX)          -lvtkmetaio$(VTK_LIBRARY_SUFFIX)          -lvtkRenderingImage$(VTK_LIBRARY_SUFFIX)         -lvtkverdict$(VTK_LIBRARY_SUFFIX) \
-lpython3
endif



convertSources = \
  convert.cpp \
  data/DataSet.cpp \
  data/PatchData.cpp \
  data/Variable.cpp \
  filter/Copy.cpp \
  filter/Intersection.cpp \
  filter/SelectValue.cpp \
  filter/SeparateResolutions.cpp \
  filter/ConvertTreeIdIntoDataField.cpp \
  input/Parser.cpp \
  input/PeanoTextPatchFileReader.cpp \
  output/PeanoWriter.cpp \
  output/VTUWriter.cpp


convert_CXXFLAGS  = $(VTK_INCLUDE) $(OPENMP_CXXFLAGS)
convert_LDADD     = -lTarch
convert_LDFLAGS   = -L$(top_builddir)/src $(OPENMP_CXXFLAGS) $(LDFLAGS) -lstdc++fs $(VTK_REQUIRED_LIBS)
convert_SOURCES   = $(convertSources)

convert_debug_CXXFLAGS  = $(VTK_INCLUDE) $(OPENMP_CXXFLAGS) -O0 -g3 -DPeanoDebug=4 
convert_debug_LDADD     = -lTarch_debug
convert_debug_LDFLAGS   = -L$(top_builddir)/src $(OPENMP_CXXFLAGS) $(LDFLAGS) -lstdc++fs $(VTK_REQUIRED_LIBS)
convert_debug_SOURCES   = $(convertSources)


