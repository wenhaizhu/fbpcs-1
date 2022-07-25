#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

import logging
import os
from dataclasses import dataclass
from typing import Any, Dict, List, Optional

from fbpcs.bolt.bolt_client import BoltClient, BoltState
from fbpcs.bolt.bolt_job import BoltCreateInstanceArgs
from fbpcs.bolt.constants import FBPCS_GRAPH_API_TOKEN
from fbpcs.pl_coordinator.exceptions import GraphAPITokenNotFound
from fbpcs.private_computation.stage_flows.private_computation_base_stage_flow import (
    PrivateComputationBaseStageFlow,
)
from fbpcs.utils.config_yaml.config_yaml_dict import ConfigYamlDict
from fbpcs.utils.config_yaml.exceptions import ConfigYamlBaseException

URL = "https://graph.facebook.com/v13.0"


@dataclass
class BoltPLGraphAPICreateInstanceArgs(BoltCreateInstanceArgs):
    instance_id: str  # used for temporary resuming solution
    study_id: str
    breakdown_key: Dict[str, str]


@dataclass
class BoltPAGraphAPICreateInstanceArgs(BoltCreateInstanceArgs):
    instance_id: str  # used for temporary resuming solution
    dataset_id: str
    timestamp: str
    attribution_rule: str
    num_containers: str


class BoltGraphAPIClient(BoltClient):
    def __init__(
        self, config: Dict[str, Any], logger: Optional[logging.Logger] = None
    ) -> None:
        """Bolt GraphAPI Client

        Args:
            - config: the graphapi section of the larger config dictionary: config["graphapi"]
            - logger: logger
        """
        self.logger: logging.Logger = (
            logging.getLogger(__name__) if logger is None else logger
        )
        self.access_token = self._get_graph_api_token(config)
        self.params = {"access_token": self.access_token}

    async def create_instance(self, instance_args: BoltCreateInstanceArgs) -> str:
        pass

    async def run_stage(
        self,
        instance_id: str,
        stage: Optional[PrivateComputationBaseStageFlow] = None,
        server_ips: Optional[List[str]] = None,
    ) -> None:
        pass

    async def update_instance(self, instance_id: str) -> BoltState:
        pass

    async def validate_results(
        self, instance_id: str, expected_result_path: Optional[str] = None
    ) -> bool:
        pass

    def _get_graph_api_token(self, config: Dict[str, Any]) -> str:
        """Get graph API token from config.yml or the {FBPCS_GRAPH_API_TOKEN} env var

        Args:
            config: dictionary representation of config.yml file

        Returns:
            the graph api token

        Raises:
            GraphAPITokenNotFound: graph api token not in config.yml and not in env var
        """
        try:
            if not isinstance(config, ConfigYamlDict):
                config = ConfigYamlDict.from_dict(config)
            self.logger.info("attempting to read graph api token from config.yml file")
            token = config["access_token"]
            self.logger.info("successfuly read graph api token from config.yml file")
        except ConfigYamlBaseException:
            self.logger.info(
                f"attempting to read graph api token from {FBPCS_GRAPH_API_TOKEN} env var"
            )
            token = os.getenv(FBPCS_GRAPH_API_TOKEN)
            if not token:
                no_token_exception = GraphAPITokenNotFound.make_error()
                self.logger.exception(no_token_exception)
                raise no_token_exception from None
            self.logger.info(
                f"successfully read graph api token from {FBPCS_GRAPH_API_TOKEN} env var"
            )
        return token
