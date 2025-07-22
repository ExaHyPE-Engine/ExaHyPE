def test_array_within_threshold(arr, lower_threshold=-1e-8, upper_threshold=1e-8):
    # Assert: kein Wert < lower_threshold
    min_val = arr.min()
    assert (
        min_val >= lower_threshold
    ), f"Array enthält Wert unter {lower_threshold}: min={min_val}"

    # Assert: kein Wert > upper_threshold
    max_val = arr.max()
    assert (
        max_val <= upper_threshold
    ), f"Array enthält Wert über {upper_threshold}: max={max_val}"
