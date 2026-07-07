void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllHorizontalDataExchangeStacks() {
{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{cell.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{face.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllPeriodicBoundaryDataStacks() {
{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{cell.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{face.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::finishAllOutstandingSendsAndReceives() {
{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::{{cell.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::{{face.logical_type_name}}StackContainer, _spacetreeId);
  // ::peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::{{face.logical_type_name}}HangingStackContainer, _spacetreeId);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer, _spacetreeId);
  // ::peano4::parallel::SpacetreeSet::finishAllOutstandingSendsAndReceives(repositories::DataRepository::{{vertex.logical_type_name}}HangingStackContainer, _spacetreeId);
{%- endfor %}
}
