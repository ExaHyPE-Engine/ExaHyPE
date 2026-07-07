void {{FULL_QUALIFIED_CLASSNAME}}::streamDataFromJoiningTreeToMasterTree(int master) {
{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::{{cell.logical_type_name}}StackContainer, _spacetreeId, master);
{%- endfor %}
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::{{face.logical_type_name}}StackContainer, _spacetreeId, master);
  // ::peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::{{face.logical_type_name}}HangingStackContainer, _spacetreeId, master);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer, _spacetreeId, master);
  // ::peano4::parallel::SpacetreeSet::streamDataFromJoiningTreeToMasterTree(repositories::DataRepository::{{vertex.logical_type_name}}HangingStackContainer, _spacetreeId, master);
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::streamDataFromSplittingTreeToNewTree(int newWorker) {
{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::{{cell.logical_type_name}}StackContainer, _spacetreeId, newWorker);
{%- endfor %}
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::{{face.logical_type_name}}StackContainer, _spacetreeId, newWorker);
  // ::peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::{{face.logical_type_name}}HangingStackContainer, _spacetreeId, newWorker);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer, _spacetreeId, newWorker);
  // ::peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree(repositories::DataRepository::{{vertex.logical_type_name}}HangingStackContainer, _spacetreeId, newWorker);
{%- endfor %}
}
