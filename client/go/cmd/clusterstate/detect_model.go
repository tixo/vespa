// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// Author: arnej

// utilities to get and manipulate node states in a storage cluster
package clusterstate

import (
	"strconv"
	"strings"

	"github.com/vespa-engine/vespa/client/go/vespa"
)

func getConfigServerHosts(s string) []string {
	if s != "" {
		return []string{s}
	}
	got, err := getOutputFromCmd(vespa.FindHome()+"/bin/vespa-print-default", "configservers")
	res := strings.Fields(got)
	if err != nil || len(res) < 1 {
		panic("bad configservers: " + got)
	}
	PutDebug("found", len(res), "configservers:", res)
	return res
}

func getConfigServerPort(i int) int {
	if i > 0 {
		return i
	}
	got, err := getOutputFromCmd(vespa.FindHome()+"/bin/vespa-print-default", "configserver_rpc_port")
	if err == nil {
		i, err = strconv.Atoi(strings.TrimSpace(got))
	}
	if err != nil || i < 1 {
		panic("bad configserver_rpc_port: " + got)
	}
	PutDebug("found configservers rpc port:", i)
	return i
}

func detectModel(opts *Options) *VespaModelConfig {
	vespa.LoadDefaultEnv()
	cfgHosts := getConfigServerHosts(opts.ConfigServerHost)
	cfgPort := getConfigServerPort(opts.ConfigServerPort)
	for _, cfgHost := range cfgHosts {
		args := []string{
			"-j",
			"-n", "cloud.config.model",
			"-i", "admin/model",
			"-p", strconv.Itoa(cfgPort),
			"-s", cfgHost,
		}
		data, err := getOutputFromCmd(vespa.FindHome()+"/bin/vespa-get-config", args...)
		parsed := parseModelConfig(data)
		if err == nil && parsed != nil {
			return parsed
		}
	}
	panic("could not get model config")
}