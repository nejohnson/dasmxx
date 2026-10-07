"""Suite entry point for the standalone framework runner."""

from run_tests import create_tool_feature_tests


def create_suite():
    return create_tool_feature_tests()
