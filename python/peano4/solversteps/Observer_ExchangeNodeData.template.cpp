void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllHorizontalDataExchangeStacks(bool symmetricDataCardinality) {
{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{cell.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
{%- endfor %}
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{face.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllHorizontalDataExchangeStacks(repositories::DataRepository::{{vertex.logical_type_name}}Stack, _spacetreeId, symmetricDataCardinality);
{%- endfor %}
}

void {{FULL_QUALIFIED_CLASSNAME}}::exchangeAllPeriodicBoundaryDataStacks() {
{%- for cell in CELLS %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{cell.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}
{%- for face in FACES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{face.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}
{%- for vertex in VERTICES %}
  ::peano4::parallel::SpacetreeSet::exchangeAllPeriodicBoundaryDataStacks(repositories::DataRepository::{{vertex.logical_type_name}}Stack, _spacetreeId);
{%- endfor %}
}
