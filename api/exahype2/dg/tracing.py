# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap


def render_tracer_update(tracer, quadrature):
    """Returns the C++ call that moves one tracer set through the nodal polynomial of a DG
    solver family, or that samples the traces of the solution on the faces of the set's plane;
    ``quadrature`` names the generated quadrature table class. A set that samples face traces
    and records them samples only when its database or its error measurement records."""
    if tracer["FACE_TRACES"]:
        fields = "".join(
            ", {}".format(field) for field in tracer["FACE_TRACES"]["DIFFERENCE_FIELDS"]
        )
        sampling = (
            "projectSolutionOntoPlane(partition, {normal}, {position});\n"
            "::exahype2::dg::traceFaceValues<NodesPerAxis, NumberOfData, {number_of_data}{fields}>(\n"
            "  partition,\n"
            "  getFaceStates(),\n"
            "  {quadrature}<double>::QuadraturePoints1D,\n"
            "  _{name},\n"
            "  {normal},\n"
            "  {position}\n"
            ");"
        ).format(
            number_of_data=tracer["NUMBER_OF_DATA"],
            fields=fields,
            name=tracer["NAME"],
            quadrature=quadrature,
            normal=tracer["FACE_TRACES"]["NORMAL"],
            position=tracer["FACE_TRACES"]["POSITION"],
        )
        records = []
        if tracer["DATABASE"]:
            records.append(
                "::exahype2::particles::isSnapshotDueOnSomeRank(_{name}, getTimeStepping().getTimeStamp(), {initial}, {last}, _{name}Database)".format(
                    name=tracer["NAME"],
                    initial=tracer["DATABASE"]["INITIAL_RECORD_TIME"],
                    last=tracer["DATABASE"]["LAST_RECORD_TIME"],
                )
            )
        if tracer["ERROR"]:
            records.append(
                "_{name}ErrorMeasurement.isSnapshotDue(getTimeStepping().getTimeStamp())".format(
                    name=tracer["NAME"]
                )
            )
        if not records:
            return sampling
        return "if ({}) {{\n{}\n}}".format(
            " or ".join(records), textwrap.indent(sampling, "  ")
        )
    fields = "".join(", {}".format(field) for field in tracer["FIELDS"])
    return (
        "::exahype2::dg::traceParticles<NodesPerAxis, NumberOfData, {number_of_data}{fields}>(\n"
        "  partition,\n"
        "  getSolution(),\n"
        "  {quadrature}<double>::QuadraturePoints1D,\n"
        "  timeStepSize,\n"
        "  _{name},\n"
        "  _{name}Forces,\n"
        "  DomainOffset,\n"
        "  DomainSize,\n"
        "  static_cast<int>(PeriodicBoundaries.to_ulong())\n"
        ");"
    ).format(
        number_of_data=tracer["NUMBER_OF_DATA"],
        fields=fields,
        name=tracer["NAME"],
        quadrature=quadrature,
    )
