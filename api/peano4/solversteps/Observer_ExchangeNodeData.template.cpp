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

void {{FULL_QUALIFIED_CLASSNAME}}::purgePeriodicBoundaryDataStacks() {
  // Cell data never crosses a periodic boundary; only faces and vertices are
  // purged and routed.
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::purgePeriodicBoundaryDataStacks(repositories::DataRepository::{{face.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::purgePeriodicBoundaryDataStacks(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer, _spacetreeId);
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllPeriodicBoundaryDataStacks() {
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

void {{FULL_QUALIFIED_CLASSNAME}}::exchangeCrossRankPeriodicData() {
  // All containers of a tree pair share one periodic tag, so every rank has
  // to process the containers in the same order for the messages to match.
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::exchangeCrossRankPeriodicData(repositories::DataRepository::{{face.logical_type_name}}StackContainer);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::exchangeCrossRankPeriodicData(repositories::DataRepository::{{vertex.logical_type_name}}StackContainer);
{%- endfor %}
}
