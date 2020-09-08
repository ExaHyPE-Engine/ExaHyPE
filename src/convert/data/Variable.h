#ifndef _VISUALISATION_DATA_VARIABLE_H_
#define _VISUALISATION_DATA_VARIABLE_H_

#include <string>


namespace visualisation {
  namespace data {
    enum class PeanoDataType {Cell_Values, Vertex_Values};
    class Variable;
  }
}


/**
 * Represents one variable that is subsequently attached to the patches of a
 * file.
 */
class visualisation::data::Variable {
  public:
	Variable(std::string name_, int dofsPerAxis_, int unknowns_, PeanoDataType type_, int dimensions_);

	const std::string   name;

	const int           dofsPerAxis;

	/**
	 * If this unknown holds 1, it is scalar, for d it is vector data, and so
	 * forth.
	 */
	const int           unknowns;

	const PeanoDataType type;

	const int           dimensions;

	// @todo Patch size (wieviel Punkte pro Dimension)
/*
	const int points;
	int totalValues;
	int mappings;
	double* mapping;
*/
	bool operator<( const visualisation::data::Variable& rhs ) const;
	bool operator==( const visualisation::data::Variable& rhs ) const;

	/**
	 * This is getTotalNumberOfDofsPerPatch() times the number of unknowns per dof.
	 */
	int getTotalNumberOfQuantitiesPerPatch() const;
	int getTotalNumberOfDofsPerPatch() const;
	int getVerticesPerAxisInCartesianMesh() const;
	int getTotalNumberOfVerticesInCartesianMesh() const;
	int getTotalNumberOfCellsInCartesianMesh() const;

	std::string toString() const;
};



#endif /* PEANOVARIABLE_H_ */
